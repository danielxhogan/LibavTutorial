#pragma once

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

#define INACTIVE_STREAM -1

typedef struct InputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext **dec_ctx;
  AVPacket *pkt;
  AVFrame *dec_frame;
  int *map;
  int nb_selected_streams;
} InputContext;

InputContext *open_input(const char *in_filename, const char *selected_streams);

void close_input(InputContext *in_ctx);
