#include "fsc.h"

FrameSizeConversionContext *fsc_ctx_alloc(AVCodecContext *enc_ctx)
{
  FrameSizeConversionContext *fsc_ctx =
    malloc(sizeof(FrameSizeConversionContext));
  if (!fsc_ctx) return NULL;

  for (int i = 0; i < SAMPLE_BUFFER_LENGTH; i++) {
    fsc_ctx->sample_buffer[i] = NULL;
  }

  fsc_ctx->sample_buffer_capacity = 0;
  fsc_ctx->nb_samples_in_buffer = 0;
  fsc_ctx->channels = enc_ctx->ch_layout.nb_channels;
  fsc_ctx->bytes_per_sample =
    av_get_bytes_per_sample(enc_ctx->sample_fmt) * fsc_ctx->channels;
  fsc_ctx->sample_fmt = enc_ctx->sample_fmt;
  fsc_ctx->frame_size = enc_ctx->frame_size;

  if (!(fsc_ctx->frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    return NULL;
  }

  fsc_ctx->frame->format = enc_ctx->sample_fmt;
  av_channel_layout_copy(&fsc_ctx->frame->ch_layout, &enc_ctx->ch_layout);
  fsc_ctx->frame->sample_rate = enc_ctx->sample_rate;
  fsc_ctx->frame->nb_samples = enc_ctx->frame_size;

  if ((av_frame_get_buffer(fsc_ctx->frame, 0)) < 0) {
    fprintf(stderr, "Failed to allocate buffers for frame.\n");
    return NULL;
  }

  return fsc_ctx;
}

int fsc_ctx_alloc_buffer(FrameSizeConversionContext *fsc_ctx, int capacity)
{
  int ret = 0;

  if (fsc_ctx->sample_buffer[0] == NULL)
  {
    if ((ret = av_samples_alloc(fsc_ctx->sample_buffer, NULL,
      fsc_ctx->channels, capacity,
      fsc_ctx->sample_fmt, 0)) < 0)
    {
      fprintf(stderr, "Failed to allocate samples buffer.\n");
      return ret;
    }

    fsc_ctx->sample_buffer_capacity = capacity;
  }
  else if (fsc_ctx->sample_buffer_capacity < capacity)
  {
    uint8_t *tmp_buffer[SAMPLE_BUFFER_LENGTH];
    for (int i = 0; i < SAMPLE_BUFFER_LENGTH; i++) {
      tmp_buffer[i] = NULL;
    }

    if ((ret = av_samples_alloc(tmp_buffer, NULL, fsc_ctx->channels,
      fsc_ctx->sample_buffer_capacity, fsc_ctx->sample_fmt, 0)) < 0)
    {
      fprintf(stderr, "Failed to allocate samples buffer.\n");
      return ret;
    }

    av_samples_copy(tmp_buffer, fsc_ctx->sample_buffer, 0, 0,
      fsc_ctx->sample_buffer_capacity, fsc_ctx->channels,
      fsc_ctx->sample_fmt);

    av_freep(&fsc_ctx->sample_buffer[0]);

    if ((ret = av_samples_alloc(fsc_ctx->sample_buffer, NULL,
      fsc_ctx->channels, capacity,
      fsc_ctx->sample_fmt, 0)) < 0)
    {
      fprintf(stderr, "Failed to allocate samples buffer.\n");
      return ret;
    }

    av_samples_copy(fsc_ctx->sample_buffer, tmp_buffer, 0, 0,
      fsc_ctx->sample_buffer_capacity, fsc_ctx->channels,
      fsc_ctx->sample_fmt);

    av_freep(&tmp_buffer[0]);
    fsc_ctx->sample_buffer_capacity = capacity;
  }

  return 0;
}

int fsc_ctx_add_samples_to_buffer(FrameSizeConversionContext *fsc_ctx,
  AVFrame *dec_frame, int nb_converted_samples)
{
  int ret = 0;

  if ((ret = fsc_ctx_alloc_buffer(fsc_ctx,
    fsc_ctx->frame_size + dec_frame->nb_samples)) < 0)
  {
    fprintf(stderr, "Failed to allocate sample buffer for fsc_ctx.\n");
    return ret;
  }

  ret = av_samples_copy(fsc_ctx->sample_buffer, dec_frame->data,
    fsc_ctx->nb_samples_in_buffer, 0, dec_frame->nb_samples,
    fsc_ctx->channels, fsc_ctx->sample_fmt);

  if (ret < 0) {
    fprintf(stderr,
      "Failed to copy samples from decoded frame to fsc_ctx->sample_buffer.\n");
    return ret;
  }

  fsc_ctx->nb_samples_in_buffer += nb_converted_samples;
  return ret;
}

int fsc_ctx_make_frame(FrameSizeConversionContext *fsc_ctx, int64_t timestamp)
{
  int ret = 0;

  if (fsc_ctx->nb_samples_in_buffer < fsc_ctx->frame_size) {
    fprintf(stderr, "Not enough samples in buffer to make a frame.\n");
    return -1;
  }

  ret = av_samples_copy(fsc_ctx->frame->data, fsc_ctx->sample_buffer, 0, 0,
    fsc_ctx->frame_size, fsc_ctx->channels, fsc_ctx->sample_fmt);

  if (ret < 0) {
    fprintf(stderr, "Failed to copy samples from buffer into encoder frame.\n");
    return ret;
  }

  fsc_ctx->frame->pts = timestamp;

  memmove(fsc_ctx->sample_buffer[0],
    fsc_ctx->sample_buffer[0] + fsc_ctx->frame_size * fsc_ctx->bytes_per_sample,
    (fsc_ctx->nb_samples_in_buffer - fsc_ctx->frame_size) * fsc_ctx->bytes_per_sample);

  fsc_ctx->nb_samples_in_buffer -= fsc_ctx->frame_size;
  return ret;
}

void fsc_ctx_free(FrameSizeConversionContext *fsc_ctx)
{
  if (!fsc_ctx) return;
  av_freep(&fsc_ctx->sample_buffer[0]);
  av_frame_free(&fsc_ctx->frame);
  free(fsc_ctx);
}
