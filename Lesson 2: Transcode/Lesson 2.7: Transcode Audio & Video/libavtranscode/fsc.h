#pragma once

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#define SAMPLE_BUFFER_LENGTH AV_NUM_DATA_POINTERS

typedef struct FrameSizeConversionContext {
  uint8_t *sample_buffer[SAMPLE_BUFFER_LENGTH];
  int sample_buffer_capacity;
  int nb_samples_in_buffer;
  int channels;
  int bytes_per_sample;
  int frame_size;
  AVFrame *frame;
  enum AVSampleFormat sample_fmt;
} FrameSizeConversionContext;

FrameSizeConversionContext *fsc_ctx_alloc(AVCodecContext *enc_ctx);

int fsc_ctx_alloc_buffer(FrameSizeConversionContext *fsc_ctx, int capacity);

int fsc_ctx_add_samples_to_buffer(FrameSizeConversionContext *fsc_ctx,
  AVFrame *dec_frame, int nb_converted_samples);

int fsc_ctx_make_frame(FrameSizeConversionContext *fsc_ctx, int64_t timestamp);

void fsc_ctx_free(FrameSizeConversionContext *fsc_ctx);
