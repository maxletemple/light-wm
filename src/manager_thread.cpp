#include "manager_thread.h"
#include <iostream>
#include "fifo_queue.h"
#include "structs.h"
#include <iomanip>
#include <cstring>
#include "window_protocol.h"
#include "window_list.h"
#include "display_object.h"
#include "text_object.h"
#include "picture_object.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

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
            std::cout << "processing new packet" << std::endl;
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
            case CMD_WIN_DESTROY: {
                if (packet.data.size() < sizeof(WinDestroy)) {
                    std::cerr << "CMD_WIN_DESTROY: payload too small" << std::endl;
                    break;
                }
                WinDestroy cmd;
                std::memcpy(&cmd, packet.data.data(), sizeof(WinDestroy));
                if (cmd.winIndex >= window_list_.size()) {
                    std::cerr << "CMD_WIN_DESTROY: index out of range" << std::endl;
                    break;
                }
                window_list_.remove(cmd.winIndex);
                std::cout << "Window " << cmd.winIndex << " destroyed" << std::endl;
                break;
            }
            case CMD_WIN_TRANSFORM: {
                if (packet.data.size() < sizeof(WinTransform)) {
                    std::cerr << "CMD_WIN_TRANSFORM: payload too small" << std::endl;
                    break;
                }
                WinTransform cmd;
                std::memcpy(&cmd, packet.data.data(), sizeof(WinTransform));
                if (cmd.winIndex >= window_list_.size()) {
                    std::cerr << "CMD_WIN_TRANSFORM: index out of range" << std::endl;
                    break;
                }
                window_list_.getWindow(cmd.winIndex)->transform(cmd.posX, cmd.posY, cmd.width, cmd.height);
                std::cout << "Window " << cmd.winIndex << " transformed to ("
                          << cmd.posX << ", " << cmd.posY << ") "
                          << cmd.width << "x" << cmd.height << std::endl;
                break;
            }
            case CMD_WIN_ORDER: {
                if (packet.data.size() < sizeof(WinOrder)) {
                    std::cerr << "CMD_WIN_ORDER: payload too small" << std::endl;
                    break;
                }
                WinOrder cmd;
                std::memcpy(&cmd, packet.data.data(), sizeof(WinOrder));
                if (cmd.winIndex >= window_list_.size() || cmd.order >= window_list_.size()) {
                    std::cerr << "CMD_WIN_ORDER: index out of range" << std::endl;
                    break;
                }
                window_list_.move(cmd.winIndex, cmd.order);
                std::cout << "Window " << cmd.winIndex << " moved to order " << cmd.order << std::endl;
                break;
            }
            case CMD_SET_OBJECT: {
                if (packet.data.size() < sizeof(ObjSet)) {
                    std::cerr << "CMD_SET_OBJECT: payload too small" << std::endl;
                    break;
                }
                ObjSet cmd;
                std::memcpy(&cmd, packet.data.data(), sizeof(ObjSet));
                if (cmd.winIndex >= window_list_.size()) {
                    std::cerr << "CMD_SET_OBJECT: winIndex out of range" << std::endl;
                    break;
                }
                const uint8_t* payload = packet.data.data() + sizeof(ObjSet);
                const size_t payloadLen = packet.data.size() - sizeof(ObjSet);

                if (packet.header.data_type == DTYPE_TEXT) {
                    if (payloadLen < sizeof(TextData)) {
                        std::cerr << "CMD_SET_OBJECT: TextData payload too small" << std::endl;
                        break;
                    }
                    TextData td;
                    std::memcpy(&td, payload, sizeof(TextData));
                    const char* str = reinterpret_cast<const char*>(payload + sizeof(TextData));
                    const size_t maxLen = payloadLen - sizeof(TextData);
                    std::string text(str, strnlen(str, maxLen));
                    auto* obj = new TextObject(cmd.x, cmd.y, cmd.width, cmd.height,
                                               td.fontSize, text);
                    window_list_.getWindow(cmd.winIndex)->setObject(cmd.objIndex, obj);
                    std::cout << "Object " << cmd.objIndex << " (text) set on window "
                              << cmd.winIndex << std::endl;
                } else if (packet.header.data_type == DTYPE_PICTURE) {
                    if (payloadLen < sizeof(PictureData)) {
                        std::cerr << "CMD_SET_OBJECT: PictureData payload too small" << std::endl;
                        break;
                    }
                    PictureData pd;
                    std::memcpy(&pd, payload, sizeof(PictureData));
                    const uint8_t* imgData = payload + sizeof(PictureData);
                    const size_t   imgLen  = payloadLen - sizeof(PictureData);

                    const char* inputFmt = (pd.format == PFMT_JPEG) ? "mjpeg" : "png_pipe";
                    const std::string scaleArg =
                        std::to_string(cmd.width) + ":" + std::to_string(cmd.height);

                    // Pipes : [0]=lecture, [1]=écriture
                    int pipeIn[2], pipeOut[2];
                    if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0) {
                        std::cerr << "CMD_SET_OBJECT: pipe() failed" << std::endl;
                        break;
                    }

                    pid_t pid = fork();
                    if (pid < 0) {
                        std::cerr << "CMD_SET_OBJECT: fork() failed" << std::endl;
                        break;
                    }

                    if (pid == 0) {
                        // Processus fils : ffmpeg
                        dup2(pipeIn[0],  STDIN_FILENO);
                        dup2(pipeOut[1], STDOUT_FILENO);
                        close(pipeIn[1]); close(pipeOut[0]);
                        close(pipeIn[0]); close(pipeOut[1]);
                        // Silencer stderr de ffmpeg
                        int devnull = open("/dev/null", O_WRONLY);
                        if (devnull >= 0) dup2(devnull, STDERR_FILENO);
                        execlp("ffmpeg", "ffmpeg",
                               "-f", inputFmt, "-i", "pipe:0",
                               "-vf", ("scale=" + scaleArg).c_str(),
                               "-f", "rawvideo", "-pix_fmt", "gray", "pipe:1",
                               nullptr);
                        _exit(1);
                    }

                    // Processus parent
                    close(pipeIn[0]); close(pipeOut[1]);
                    // Écriture des données image sur stdin de ffmpeg
                    write(pipeIn[1], imgData, imgLen);
                    close(pipeIn[1]);
                    // Lecture des pixels décodés
                    const size_t expected = (size_t)cmd.width * cmd.height;
                    std::vector<uint8_t> decoded(expected);
                    size_t total = 0;
                    while (total < expected) {
                        ssize_t n = read(pipeOut[0], decoded.data() + total, expected - total);
                        if (n <= 0) break;
                        total += n;
                    }
                    close(pipeOut[0]);
                    waitpid(pid, nullptr, 0);

                    if (total < expected) {
                        std::cerr << "CMD_SET_OBJECT: ffmpeg decoded only " << total
                                  << "/" << expected << " bytes" << std::endl;
                        break;
                    }
                    auto* obj = new PictureObject(cmd.x, cmd.y, cmd.width, cmd.height,
                                                  decoded.data(), expected);
                    window_list_.getWindow(cmd.winIndex)->setObject(cmd.objIndex, obj);
                    std::cout << "Object " << cmd.objIndex << " (picture) set on window "
                              << cmd.winIndex << std::endl;
                } else {
                    std::cout << "CMD_SET_OBJECT: data_type=" << packet.header.data_type
                              << " not yet implemented" << std::endl;
                }
                break;
            }
            case CMD_RM_OBJECT: {
                if (packet.data.size() < sizeof(ObjRemove)) {
                    std::cerr << "CMD_RM_OBJECT: payload too small" << std::endl;
                    break;
                }
                ObjRemove cmd;
                std::memcpy(&cmd, packet.data.data(), sizeof(ObjRemove));
                if (cmd.winIndex >= window_list_.size()) {
                    std::cerr << "CMD_RM_OBJECT: winIndex out of range" << std::endl;
                    break;
                }
                window_list_.getWindow(cmd.winIndex)->removeObject(cmd.objIndex);
                std::cout << "Object " << cmd.objIndex << " removed from window " << cmd.winIndex << std::endl;
                break;
            }
            default:
                break;
            }
        }
    }
}