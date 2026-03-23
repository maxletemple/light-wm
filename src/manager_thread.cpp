#include "manager_thread.h"
#include <iostream>
#include "fifo_queue.h"
#include "structs.h"
#include <iomanip>
#include <cstring>
#include "window_protocol.h"
#include "window_list.h"

void print_bytes(const std::vector<uint8_t> &data)
{
    for (size_t i = 0; i < data.size(); i++)
    {
        if (i % 16 == 0 && i != 0)
            std::cout << '\n';
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]) << ' ';
    }
    std::cout << std::dec << '\n';
}

void ManagerThread::run()
{
    std::cout << "Thread started" << std::endl;

    while (1)
    {
        if (network_queue_.size() > 0)
        {
            RawPacket packet = network_queue_.pop();

            switch (packet.header.command)
            {
            case CMD_PING:
                std::cout << "Received ping" << std::endl;
                break;
            case CMD_WIN_CREATE: {
                if (packet.data.size() < sizeof(WinCreate)) {
                    std::cerr << "CMD_WIN_CREATE: payload too small" << std::endl;
                    break;
                }
                WinCreate cmd;
                std::memcpy(&cmd, packet.data.data(), sizeof(WinCreate));
                Window* win = new Window(cmd.posX, cmd.posY, cmd.width, cmd.height, cmd.backgroundColor);
                window_list_.add(win);
                std::cout << "Window created at (" << cmd.posX << ", " << cmd.posY << ")" << std::endl;
                break;
            }
            default:
                break;
            }
        }
    }
}