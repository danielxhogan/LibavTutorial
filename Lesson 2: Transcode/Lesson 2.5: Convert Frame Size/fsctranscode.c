#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

typedef struct InputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *dec_ctx;
  AVFrame *dec_frame;
  int stream_idx;
} InputContext;

InputContext *open_input(const char *in_filename, int stream_idx)
{
  int ret = 0;
  AVStream *in_stream;
  const AVCodec *dec;

  InputContext *ctx = malloc(sizeof(InputContext));
  if (!ctx) {
    ret = ENOMEM;
    return NULL;
  }

  ctx->fmt_ctx = NULL;
  ctx->dec_ctx = NULL;
  ctx->dec_frame = NULL;
  ctx->stream_idx = stream_idx;

  if ((ret = avformat_open_input(&ctx->fmt_ctx, in_filename, NULL, NULL)) < 0) {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", in_filename);
    return NULL;
  }

  if ((ret = avformat_find_stream_info(ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    return NULL;
  }

  in_stream = ctx->fmt_ctx->streams[stream_idx];

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder.\n");
    ret = AVERROR(EINVAL);
    return NULL;
  }

  if (!(ctx->dec_ctx = avcodec_alloc_context3(dec))) {
    fprintf(stderr, "Failed to allocate decoder context.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  if ((ret = avcodec_parameters_to_context(ctx->dec_ctx, in_stream->codecpar)) < 0) {
    fprintf(stderr, "Failed to copy codec parameters to decoder context.\n");
    return NULL;
  }

  if ((ret = avcodec_open2(ctx->dec_ctx, dec, NULL)) < 0) {
    fprintf(stderr, "Failed to open decoder.\n");
    return NULL;
  }

  if (!(ctx->dec_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  return ctx;
}

void close_input(InputContext *ctx)
{
  if (!ctx) return;
  avformat_close_input(&ctx->fmt_ctx);
  avcodec_free_context(&ctx->dec_ctx);
  av_frame_free(&ctx->dec_frame);
  free(ctx);
}

typedef struct OutputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *enc_ctx;
} OutputContext;

OutputContext *open_output(AVCodecContext *dec_ctx,
  const char *codec, const char *out_filename,
  AVDictionary *fmt_metadata, AVDictionary *stream_metadata)
{
  int ret;
  AVStream *out_stream;
  const AVCodec *enc;

  OutputContext *ctx = malloc(sizeof(OutputContext));
  if (!ctx) {
    ret = ENOMEM;
    return NULL;
  }

  ctx->fmt_ctx = NULL;
  ctx->enc_ctx = NULL;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder.\n");
    ret = AVERROR_UNKNOWN;
    return NULL;
  }

  if (!(ctx->enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder context.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  if ((ret = av_channel_layout_copy(&ctx->enc_ctx->ch_layout,
    &dec_ctx->ch_layout)) < 0)
  {
    fprintf(stderr, "Failed to set channel layout on encoder.\n");
    return NULL;
  }

  ctx->enc_ctx->sample_fmt = dec_ctx->sample_fmt;
  ctx->enc_ctx->sample_rate = dec_ctx->sample_rate;
  ctx->enc_ctx->time_base = (AVRational) {1, ctx->enc_ctx->sample_rate};
  ctx->enc_ctx->bit_rate = dec_ctx->bit_rate;

  if ((ret = avcodec_open2(ctx->enc_ctx, enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    return NULL;
  }

  if ((ret = avformat_alloc_output_context2(&ctx->fmt_ctx,
    NULL, NULL, out_filename)))
  {
    fprintf(stderr, "Failed to allocate output format context.\n");
    return NULL;
  }

  if ((ret = av_dict_copy(&ctx->fmt_ctx->metadata,
    fmt_metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy file metadata.\n");
    return NULL;
  }

  if (!(out_stream = avformat_new_stream(ctx->fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate new output stream.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    stream_metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy audio metadata.\n");
    return NULL;
  }

  if ((ret =
    avcodec_parameters_from_context(out_stream->codecpar, ctx->enc_ctx)))
  {
    fprintf(stderr,
      "Failed to copy codec parameters from encoder context to stream.\n");
      return NULL;
  }

  out_stream->time_base = ctx->enc_ctx->time_base;

  if (!(ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if ((ret = avio_open(&ctx->fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0)
    {
      fprintf(stderr, "Failed to open output file.\n");
      return NULL;
    }
  }

  if ((ret = avformat_write_header(ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to write header for output file.\n");
    return NULL;
  }

  return ctx;
}

void close_output(OutputContext *ctx)
{
  if (!ctx) return;
  if (ctx->fmt_ctx && !(ctx->fmt_ctx->flags & AVFMT_NOFILE))
    avio_closep(&ctx->fmt_ctx->pb);
  avformat_free_context(ctx->fmt_ctx);
  avcodec_free_context(&ctx->enc_ctx);
}

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
  AVFrame *dec_frame)
{
  int ret = 0;

  // printf("in_ctx->dec_frame->nb_samples: %d\n", in_ctx->dec_frame->nb_samples);
  // printf("out_ctx->enc_ctx->frame_size: %d\n", out_ctx->enc_ctx->frame_size);

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

  fsc_ctx->nb_samples_in_buffer += dec_frame->nb_samples;
  return ret;
}

int fsc_ctx_make_frame(FrameSizeConversionContext *fsc_ctx)
{
  int ret = 0;

  if (fsc_ctx->nb_samples_in_buffer < fsc_ctx->frame_size) {
    fprintf(stderr, "Not enough samples in buffer to make a frame.\n");
    return -1;
  }

  ret = av_samples_copy(fsc_ctx->frame->data, fsc_ctx->sample_buffer, 0, 0,
    fsc_ctx->frame_size, fsc_ctx->channels,
    fsc_ctx->sample_fmt);

  if (ret < 0) {
    fprintf(stderr, "Failed to copy samples from buffer into encoder frame.\n");
    return ret;
  }

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

int main(int argc, char **argv)
{
  const char *in_filename, *out_filename, *codec;
  int ret = 0;
  AVPacket *pkt = NULL;

  InputContext *in_ctx = NULL;
  OutputContext *out_ctx = NULL;

  FrameSizeConversionContext *fsc_ctx = NULL;

  if (argc != 4) {
    printf("\nUsage: %s <input file> <output file> <codec>\n\n\t"
      "This example will take in a file with an audio stream, \n\t"
      "transcode the video using the specified codec, and output \n\t"
      "the audio stream into a new file.\n\n", argv[0]);
    return 0;
  }

  in_filename = argv[1];
  out_filename = argv[2];
  codec = argv[3];

  if (!(in_ctx = open_input(in_filename, 1))) {
    fprintf(stderr, "Failed to open input.\n");
    goto end;
  }

  if (!(out_ctx = open_output(in_ctx->dec_ctx,
    codec, out_filename, in_ctx->fmt_ctx->metadata,
    in_ctx->fmt_ctx->streams[in_ctx->stream_idx]->metadata)))
  {
    fprintf(stderr, "Failed to open output.\n");
    goto end;
  }
  
  if (!(fsc_ctx = fsc_ctx_alloc(out_ctx->enc_ctx))) {
    fprintf(stderr, "Failed to initialize FrameSizeConversionContext.\n");
    ret = ENOMEM;
    goto end;
  }

  if (!(pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  while ((ret = av_read_frame(in_ctx->fmt_ctx, pkt)) >= 0)
  {
    if (!(pkt->stream_index == in_ctx->stream_idx)) {
      av_packet_unref(pkt);
      continue;
    }

    if ((ret = avcodec_send_packet(in_ctx->dec_ctx, pkt)) < 0) {
      fprintf(stderr, "Failed to send packet to decoder.\n");
      goto end;
    }

    while ((ret =
      avcodec_receive_frame(in_ctx->dec_ctx, in_ctx->dec_frame)) >= 0)
    {
      if ((ret = fsc_ctx_add_samples_to_buffer(fsc_ctx, in_ctx->dec_frame)) < 0)
      {
        fprintf(stderr, "Failed to add samples to buffer.\n");
        goto end;
      }

      while (fsc_ctx->nb_samples_in_buffer >= out_ctx->enc_ctx->frame_size)
      {
        if ((ret = fsc_ctx_make_frame(fsc_ctx)) < 0) {
          fprintf(stderr, "Failed to make frame for encoder.\n");
          goto end;
        }

        if ((ret = avcodec_send_frame(out_ctx->enc_ctx, fsc_ctx->frame)) < 0)
        {
          fprintf(stderr, "Failed to send frame to encoder.\n");
          goto end;
        }

        while ((ret = avcodec_receive_packet(out_ctx->enc_ctx, pkt)) >= 0)
        {
          pkt->stream_index = 0;

          if ((ret = av_interleaved_write_frame(out_ctx->fmt_ctx, pkt)) < 0) {
            fprintf(stderr, "Failed to write packet to file.\n");
            goto end;
          }
        }

        if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
          fprintf(stderr, "Failed to receive packet from encoder.\n");
          goto end;
        }
        
      }
    }

    if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
      fprintf(stderr, "Failed to receive frame from decoder.\n");
      goto end;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to read frame.\n");
    goto end;
  }

  if ((ret = avcodec_send_frame(out_ctx->enc_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to send frame to encoder.\n");
    goto end;
  }

  if ((ret = av_write_trailer(out_ctx->fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  close_input(in_ctx);
  close_output(out_ctx);
  fsc_ctx_free(fsc_ctx);
  av_packet_free(&pkt);

  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return ret;
  }

  return 0;
}
