#pragma once

#include "fifo_queue.h"
#include "structs.h"
#include "window_list.h"

class ManagerThread {
    private:
        FifoQueue<RawPacket>& network_queue_;
        WindowList& window_list_;

    public:
    ManagerThread(FifoQueue<RawPacket>& network_queue, WindowList& window_list)
        : network_queue_(network_queue), window_list_(window_list)
    {}

    void run();
};