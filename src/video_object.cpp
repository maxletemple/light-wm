#include "video_object.h"
#include <iostream>
#include <chrono>

VideoObject::VideoObject(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                         const uint8_t* videoData, size_t dataLen)
    : DisplayObject(x, y, width, height, nullptr, 0),
      video_data_(videoData, videoData + dataLen),
      data_pos_(0),
      parser_ctx_(nullptr),
      codec_ctx_(nullptr),
      packet_(nullptr),
      frame_(nullptr),
      initialized_(false)
{
    if (!initializeDecoder())
        std::cerr << "VideoObject: Failed to initialize decoder\n";
}

VideoObject::~VideoObject()
{
    cleanupDecoder();
}

bool VideoObject::initializeDecoder()
{
    // Prefer hardware-accelerated decoders available on Raspberry Pi.
    const AVCodec* codec = avcodec_find_decoder_by_name("h264_mmal");
    if (!codec) codec = avcodec_find_decoder_by_name("h264_v4l2m2m");
    if (codec)
        std::cerr << "VideoObject: using hardware decoder " << codec->name << "\n";
    else {
        std::cerr << "VideoObject: no hardware decoder found, falling back to software\n";
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    }
    if (!codec) {
        std::cerr << "VideoObject: H.264 decoder not found\n";
        return false;
    }

    // The parser extracts complete NAL units from the raw Annex-B bytestream
    // and also injects SPS/PPS into the codec context.
    parser_ctx_ = av_parser_init(AV_CODEC_ID_H264);
    if (!parser_ctx_) {
        std::cerr << "VideoObject: Could not init H.264 parser\n";
        return false;
    }

    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        std::cerr << "VideoObject: Could not allocate codec context\n";
        cleanupDecoder();
        return false;
    }

    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        std::cerr << "VideoObject: Could not open codec\n";
        cleanupDecoder();
        return false;
    }

    packet_ = av_packet_alloc();
    frame_  = av_frame_alloc();
    if (!packet_ || !frame_) {
        std::cerr << "VideoObject: Could not allocate packet/frame\n";
        cleanupDecoder();
        return false;
    }

    initialized_ = true;
    return true;
}

void VideoObject::cleanupDecoder()
{
    if (packet_)     av_packet_free(&packet_);
    if (frame_)      av_frame_free(&frame_);
    if (codec_ctx_)  avcodec_free_context(&codec_ctx_);
    if (parser_ctx_) { av_parser_close(parser_ctx_); parser_ctx_ = nullptr; }
}

void VideoObject::rewind()
{
    data_pos_ = 0;
    avcodec_flush_buffers(codec_ctx_);
    av_parser_close(parser_ctx_);
    parser_ctx_ = av_parser_init(AV_CODEC_ID_H264);
}

std::vector<uint8_t> VideoObject::extractYPlane()
{
    // All three formats share the same Y-plane layout (full-res luma).
    // NV12 is the typical output of h264_v4l2m2m.
    if (frame_->format != AV_PIX_FMT_YUV420P &&
        frame_->format != AV_PIX_FMT_YUVJ420P &&
        frame_->format != AV_PIX_FMT_NV12) {
        std::cerr << "VideoObject: unsupported pixel format " << frame_->format << "\n";
        av_frame_unref(frame_);
        return {};
    }

    std::vector<uint8_t> result(static_cast<size_t>(width) * height);
    const int fw = frame_->width, fh = frame_->height;
    for (int dy = 0; dy < height; ++dy) {
        const int sy = dy * fh / height;
        for (int dx = 0; dx < width; ++dx)
            result[dy * width + dx] =
                frame_->data[0][sy * frame_->linesize[0] + dx * fw / width];
    }
    av_frame_unref(frame_);
    return result;
}

std::vector<uint8_t> VideoObject::getPixels()
{
    if (!initialized_) return {};

    const auto t0 = std::chrono::steady_clock::now();

    // Two passes max: one normal run, one after rewinding at EOF.
    for (int pass = 0; pass < 2; ++pass) {

        // Inner loop: feed NAL units until a frame comes out.
        while (true) {
            // Drain any frame already waiting in the decoder.
            int ret = avcodec_receive_frame(codec_ctx_, frame_);
            if (ret == 0) return extractYPlane();
            if (ret != AVERROR(EAGAIN)) break;  // unexpected error — rewind

            if (data_pos_ >= video_data_.size()) {
                // End of stream: flush buffered frames, then rewind.
                avcodec_send_packet(codec_ctx_, nullptr);
                ret = avcodec_receive_frame(codec_ctx_, frame_);
                if (ret == 0) return extractYPlane();
                break;  // rewind
            }

            // Parse the next NAL unit out of the raw Annex-B buffer.
            uint8_t* out_data = nullptr;
            int      out_size = 0;
            int consumed = av_parser_parse2(
                parser_ctx_, codec_ctx_,
                &out_data, &out_size,
                video_data_.data() + data_pos_,
                static_cast<int>(video_data_.size() - data_pos_),
                AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

            if (consumed <= 0) { data_pos_ = video_data_.size(); continue; }
            data_pos_ += consumed;

            if (out_size > 0) {
                // packet_->data points into the parser's internal buffer;
                // valid until the next av_parser_parse2 call.
                packet_->data = out_data;
                packet_->size = out_size;
                avcodec_send_packet(codec_ctx_, packet_);
            }
        }

        rewind();
        if (!parser_ctx_) return {};  // av_parser_init failed
    }

    return {};
}
