import socket
import struct

HOST = "minitel"
PORT = 1710

# RawPacketHeader: uint32_t size, uint8_t command, uint16_t flags, uint8_t data_type
HEADER_FORMAT = "<IBHB"  # little-endian, 4+1+2+1 = 8 octets

# WinCreate: uint16_t posX, posY, width, height + uint8_t backgroundColor
WIN_CREATE_FORMAT = "<HHHHB"

CMD_PING      = 0x0
CMD_WIN_CREATE = 0x1

def make_packet(command, data=b"", flags=0, data_type=0):
    header = struct.pack(HEADER_FORMAT, len(data), command, flags, data_type)
    return header + data

def make_win_create(posX, posY, width, height, backgroundColor=0):
    data = struct.pack(WIN_CREATE_FORMAT, posX, posY, width, height, backgroundColor)
    return make_packet(CMD_WIN_CREATE, data)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))

    # Ping
    s.sendall(make_packet(CMD_PING))
    print("Ping envoyé")

    # Création d'une fenêtre
    s.sendall(make_win_create(0, 150, 200, 200, backgroundColor=255))
    print("Window create envoyé")
