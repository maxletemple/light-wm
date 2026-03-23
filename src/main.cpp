#include <iostream>
#include <config.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include "fifo_queue.h"
#include "manager_thread.h"
#include "display_thread.h"
#include "structs.h"

// void data_thread(FifoQueue<std::string>& fifo){
//     std::cout << "Data thread started" << std::endl;

//     while (1) {
//         if (fifo.size() > 0) {
//             std::cout << "Written from second thread :" << fifo.pop();
//         }
//     }

//     return;
// }

int main() {

    FifoQueue<RawPacket> fifo;
    WindowList window_list;
    ManagerThread managerThread(std::ref(fifo), std::ref(window_list));
    std::thread dataThread(&ManagerThread::run, std::ref(managerThread));

    DisplayThread displayThread(window_list);
    std::thread displayThreadHandle(&DisplayThread::run, std::ref(displayThread));

    int server_fd, client_fd;
    struct sockaddr_in address;

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

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*) &address, (socklen_t*) &address_len);

        while (1) {
            struct RawPacketHeader header;
            int recv_size = recv(client_fd, &header, sizeof(header), 0);
            if (recv_size <= 0) break;
            std::cout << "received " << recv_size << std::endl;
            RawPacket packet;
            packet.header = header;
            if (header.size > 0) {
                std::cout << "Reading data" << std::endl;
                packet.data.resize(header.size);
                recv(client_fd, packet.data.data(), header.size, 0);
            }

            fifo.push(packet);
        }

        close(client_fd);
    }

    dataThread.join();
    displayThreadHandle.join();

    return 0;
}