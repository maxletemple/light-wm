import socket
import struct

HOST = "minitel"
PORT = 1710

# RawPacketHeader: uint32_t size, uint8_t command, uint16_t flags, uint8_t data_type
HEADER_FORMAT = "<IBHB"  # little-endian, 4+1+2+1 = 8 octets

CMD_PING         = 0x0
CMD_WIN_CREATE   = 0x1
CMD_WIN_DESTROY  = 0x2
CMD_WIN_TRANSFORM = 0x3
CMD_WIN_ORDER    = 0x4

MENU = """
--- Commandes disponibles ---
  0  PING
  1  WIN_CREATE   (posX, posY, width, height, backgroundColor)
  2  WIN_DESTROY  (winIndex)
  3  WIN_TRANSFORM (winIndex, posX, posY, width, height)
  4  WIN_ORDER    (winIndex, order)
  q  Quitter
-----------------------------"""


def make_packet(command, data=b"", flags=0, data_type=0):
    header = struct.pack(HEADER_FORMAT, len(data), command, flags, data_type)
    return header + data


def make_win_create(posX, posY, width, height, backgroundColor=0):
    data = struct.pack("<HHHHB", posX, posY, width, height, backgroundColor)
    return make_packet(CMD_WIN_CREATE, data)


def make_win_destroy(winIndex):
    data = struct.pack("<H", winIndex)
    return make_packet(CMD_WIN_DESTROY, data)


def make_win_transform(winIndex, posX, posY, width, height):
    data = struct.pack("<HHHHH", winIndex, posX, posY, width, height)
    return make_packet(CMD_WIN_TRANSFORM, data)


def make_win_order(winIndex, order):
    data = struct.pack("<HH", winIndex, order)
    return make_packet(CMD_WIN_ORDER, data)


def prompt_int(label, default=None):
    suffix = f" [{default}]" if default is not None else ""
    while True:
        raw = input(f"  {label}{suffix}: ").strip()
        if raw == "" and default is not None:
            return default
        try:
            return int(raw)
        except ValueError:
            print(f"  Entier attendu.")


def interactive_loop(sock):
    print(f"Connecté à {HOST}:{PORT}")
    while True:
        print(MENU)
        choix = input("Choix : ").strip().lower()

        if choix == "q":
            print("Au revoir.")
            break

        elif choix == "0":
            sock.sendall(make_packet(CMD_PING))
            print("→ PING envoyé")

        elif choix == "1":
            posX  = prompt_int("posX", 0)
            posY  = prompt_int("posY", 0)
            width = prompt_int("width", 100)
            height = prompt_int("height", 100)
            bg    = prompt_int("backgroundColor (0-255)", 0)
            sock.sendall(make_win_create(posX, posY, width, height, bg))
            print(f"→ WIN_CREATE envoyé ({posX},{posY} {width}x{height} bg={bg})")

        elif choix == "2":
            idx = prompt_int("winIndex")
            sock.sendall(make_win_destroy(idx))
            print(f"→ WIN_DESTROY envoyé (index={idx})")

        elif choix == "3":
            idx    = prompt_int("winIndex")
            posX   = prompt_int("posX", 0)
            posY   = prompt_int("posY", 0)
            width  = prompt_int("width", 100)
            height = prompt_int("height", 100)
            sock.sendall(make_win_transform(idx, posX, posY, width, height))
            print(f"→ WIN_TRANSFORM envoyé (index={idx} {posX},{posY} {width}x{height})")

        elif choix == "4":
            idx   = prompt_int("winIndex")
            order = prompt_int("order")
            sock.sendall(make_win_order(idx, order))
            print(f"→ WIN_ORDER envoyé (index={idx} → position {order})")

        else:
            print("Commande inconnue.")


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    try:
        s.connect((HOST, PORT))
        interactive_loop(s)
    except KeyboardInterrupt:
        print("\nInterrompu.")
    except ConnectionRefusedError:
        print(f"Connexion refusée ({HOST}:{PORT})")
