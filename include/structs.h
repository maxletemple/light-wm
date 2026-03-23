#pragma once

#include <cstdint>
#include <vector>

#pragma pack(push, 1)

struct RawPacketHeader {
    uint32_t size;
    uint8_t command;
    uint16_t flags;
    uint8_t data_type;
};

struct RawPacket {
    struct RawPacketHeader header;
    std::vector<u_int8_t> data;
};

struct WinCreate {
    uint16_t posX;
    uint16_t posY;
    uint16_t width;
    uint16_t height;
    uint8_t backgroundColor;
};

#pragma pack(pop)