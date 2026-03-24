#pragma once

constexpr int CMD_PING = 0x0;
constexpr int CMD_WIN_CREATE = 0x1;
constexpr int CMD_WIN_DESTROY = 0x2;
constexpr int CMD_WIN_TRANSFORM = 0x3;
constexpr int CMD_WIN_ORDER = 0x4;
constexpr int CMD_SET_OBJECT = 0x5;
constexpr int CMD_RM_OBJECT = 0x6;

constexpr int DTYPE_TEXT = 0x0;
constexpr int DTYPE_PICTURE = 0x1;
constexpr int DTYPE_VIDEO = 0x2;