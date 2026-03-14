#include <iostream>
#include <config.hpp>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include "fifo_queue.h"

void data_thread(FifoQueue<std::string>& fifo){
    std::cout << "Data thread started" << std::endl;

    while (1) {
        if (fifo.size() > 0) {
            std::cout << "Written from second thread :" << fifo.pop();
        }
    }

    return;
}

int main() {

    FifoQueue<std::string> fifo;

    std::thread dataThread(data_thread, std::ref(fifo));

    int server_fd, client_fd;
    struct sockaddr_in address;
    char buffer[1024] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Error during socket creation" << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        std::cerr << "Error during socket binding" << std::endl;
        return -1;
    }

    listen(server_fd, 1);
    int address_len = sizeof(address);
    client_fd = accept(server_fd, (struct sockaddr*) &address, (socklen_t*) &address_len);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int recv_size = recv(client_fd, buffer, sizeof(buffer), 0);

        fifo.push(std::string(buffer));
    }

    dataThread.join();

    return 0;
}