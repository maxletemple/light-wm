#pragma once

#include "display_object.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

class VideoObject : public DisplayObject {
    std::vector<uint8_t>  video_data_;
    size_t                data_pos_;
    AVCodecParserContext* parser_ctx_;
    AVCodecContext*       codec_ctx_;
    AVPacket*             packet_;
    AVFrame*              frame_;
    bool                  initialized_;

public:
    VideoObject(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
               const uint8_t* videoData, size_t dataLen);
    ~VideoObject();
    bool needRefresh()     override { return initialized_; }
    bool isDirty()   const override { return initialized_; }
    std::vector<uint8_t> getPixels() override;

private:
    bool initializeDecoder();
    void cleanupDecoder();
    void rewind();
    std::vector<uint8_t> extractYPlane();
};
