#include <iostream>
#include <chrono>
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
            auto packet_start = std::chrono::steady_clock::now();
            size_t packet_bytes = recv_size;
            RawPacket packet;
            packet.header = header;
            if (header.size > 0) {
                packet.data.resize(header.size);
                size_t received = 0;
                while (received < header.size) {
                    int n = recv(client_fd, packet.data.data() + received,
                                 header.size - received, 0);
                    if (n <= 0) goto next_client;
                    received += n;
                }
                packet_bytes += received;
            }

            auto elapsed = std::chrono::steady_clock::now() - packet_start;
            double secs = std::chrono::duration<double>(elapsed).count();
            double kbps = secs > 0 ? (packet_bytes / 1024.0) / secs : 0;
            std::cout << "Paquet reçu : " << packet_bytes << " octets en "
                      << secs << " s → " << kbps << " Ko/s" << std::endl;

            fifo.push(packet);
        }

        next_client:
        close(client_fd);
    }

    dataThread.join();
    displayThreadHandle.join();

    return 0;
}