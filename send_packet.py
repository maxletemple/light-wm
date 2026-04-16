import socket
import struct
import os

HOST = "minitel"
PORT = 1710

# RawPacketHeader: uint32_t size, uint8_t command, uint16_t flags, uint8_t data_type
HEADER_FORMAT = "<IBHB"  # little-endian, 4+1+2+1 = 8 octets

CMD_PING          = 0x0
CMD_WIN_CREATE    = 0x1
CMD_WIN_DESTROY   = 0x2
CMD_WIN_TRANSFORM = 0x3
CMD_WIN_ORDER     = 0x4
CMD_SET_OBJECT    = 0x5
CMD_RM_OBJECT     = 0x6

DTYPE_TEXT    = 0x0
DTYPE_PICTURE = 0x1
DTYPE_VIDEO   = 0x2

PFMT_JPEG = 0x0
PFMT_PNG  = 0x1

VFMT_H264 = 0x0

MENU = """
--- Commandes disponibles ---
  0  PING
  1  WIN_CREATE    (posX, posY, width, height, backgroundColor)
  2  WIN_DESTROY   (winIndex)
  3  WIN_TRANSFORM (winIndex, posX, posY, width, height)
  4  WIN_ORDER     (winIndex, order)
  5  SET_TEXT      (winIndex, objIndex, x, y, width, height, fontSize, texte)
  6  RM_OBJECT     (winIndex, objIndex)
  7  SET_PICTURE   (winIndex, objIndex, x, y, width, height, chemin)
  8  SET_VIDEO     (winIndex, objIndex, x, y, width, height, chemin.h264)
  t  QUICK TEST    (crée fenêtre 500x500 + mire_ortf.jpeg)
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


def make_set_text(winIndex, objIndex, x, y, width, height, fontSize, text):
    # ObjSet + TextData + chaîne null-terminée
    obj_set  = struct.pack("<HHHHHH", winIndex, objIndex, x, y, width, height)
    text_hdr = struct.pack("<H", fontSize)
    text_bytes = text.encode("ascii") + b"\x00"
    data = obj_set + text_hdr + text_bytes
    return make_packet(CMD_SET_OBJECT, data, data_type=DTYPE_TEXT)


def make_rm_object(winIndex, objIndex):
    data = struct.pack("<HH", winIndex, objIndex)
    return make_packet(CMD_RM_OBJECT, data)


def make_set_video(winIdx, objIdx, x, y, width, height, path):
    with open(path, "rb") as f:
        video_bytes = f.read()
    obj_set   = struct.pack("<HHHHHH", winIdx, objIdx, x, y, width, height)
    video_hdr = struct.pack("<B", VFMT_H264)
    data = obj_set + video_hdr + video_bytes
    return make_packet(CMD_SET_OBJECT, data, data_type=DTYPE_VIDEO)

def make_set_picture(winIdx, objIdx, x, y, width, height, path):
    with open(path, "rb") as f:
        image_bytes = f.read()
    ext = os.path.splitext(path)[1].lower()
    fmt = PFMT_JPEG if ext in (".jpg", ".jpeg") else PFMT_PNG
    obj_set     = struct.pack("<HHHHHH", winIdx, objIdx, x, y, width, height)
    picture_hdr = struct.pack("<B", fmt)
    data = obj_set + picture_hdr + image_bytes
    return make_packet(CMD_SET_OBJECT, data, data_type=DTYPE_PICTURE)


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

        elif choix == "5":
            winIdx  = prompt_int("winIndex")
            objIdx  = prompt_int("objIndex", 0)
            x       = prompt_int("x", 0)
            y       = prompt_int("y", 0)
            width   = prompt_int("width", 100)
            height  = prompt_int("height", 20)
            fontSize = prompt_int("fontSize", 8)
            text    = input("  texte: ")
            sock.sendall(make_set_text(winIdx, objIdx, x, y, width, height, fontSize, text))
            print(f"→ SET_TEXT envoyé (win={winIdx} obj={objIdx} \"{text}\" fontSize={fontSize})")

        elif choix == "6":
            winIdx = prompt_int("winIndex")
            objIdx = prompt_int("objIndex")
            sock.sendall(make_rm_object(winIdx, objIdx))
            print(f"→ RM_OBJECT envoyé (win={winIdx} obj={objIdx})")

        elif choix == "7":
            winIdx  = prompt_int("winIndex")
            objIdx  = prompt_int("objIndex", 0)
            x       = prompt_int("x", 0)
            y       = prompt_int("y", 0)
            width   = prompt_int("width", 100)
            height  = prompt_int("height", 100)
            path    = input("  chemin du fichier (jpg/png): ").strip()
            try:
                sock.sendall(make_set_picture(winIdx, objIdx, x, y, width, height, path))
                print(f"→ SET_PICTURE envoyé (win={winIdx} obj={objIdx} \"{path}\")")
            except FileNotFoundError:
                print(f"  Fichier introuvable : {path}")

        elif choix == "8":
            winIdx  = prompt_int("winIndex")
            objIdx  = prompt_int("objIndex", 0)
            x       = prompt_int("x", 0)
            y       = prompt_int("y", 0)
            width   = prompt_int("width", 320)
            height  = prompt_int("height", 240)
            path    = input("  chemin du fichier H.264 brut (.h264): ").strip()
            try:
                sock.sendall(make_set_video(winIdx, objIdx, x, y, width, height, path))
                print(f"→ SET_VIDEO envoyé (win={winIdx} obj={objIdx} \"{path}\")")
            except FileNotFoundError:
                print(f"  Fichier introuvable : {path}")

        elif choix == "t":
            path = "mire_ortf.jpeg"
            try:
                sock.sendall(make_win_create(0, 0, 556, 512, 0))
                print("→ WIN_CREATE envoyé (0,0 500x500 bg=0)")
                sock.sendall(make_set_picture(0, 0, 0, 0, 556, 512, path))
                print(f"→ SET_PICTURE envoyé (win=0 obj=0 \"{path}\")")
            except FileNotFoundError:
                print(f"  Fichier introuvable : {path}")

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
