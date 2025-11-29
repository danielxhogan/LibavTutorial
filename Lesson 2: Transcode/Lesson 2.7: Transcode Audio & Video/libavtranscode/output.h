#pragma once

#include "input.h"
#include "hdr.h"
#include "fsc.h"

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

typedef struct OutputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext **enc_ctx;
  SwrContext **swr_ctx;
  FrameSizeConversionContext **fsc_ctx;
  AVFrame **swr_frame;
  AVPacket *enc_pkt;
  uint64_t *nb_samples_encoded;
  int nb_selected_streams;
} OutputContext;

OutputContext *open_output(InputContext *in_ctx,
  const char *out_filename, char *vcodec, char *acodec);

void close_output(OutputContext *out_ctx);
