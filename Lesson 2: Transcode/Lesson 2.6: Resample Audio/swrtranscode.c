#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>

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
    goto end;
  }

  ctx->fmt_ctx = NULL;
  ctx->dec_ctx = NULL;
  ctx->dec_frame = NULL;
  ctx->stream_idx = stream_idx;

  if ((ret = avformat_open_input(&ctx->fmt_ctx, in_filename, NULL, NULL)) < 0) {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", in_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  in_stream = ctx->fmt_ctx->streams[stream_idx];

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder.\n");
    ret = AVERROR(EINVAL);
    goto end;
  }

  if (!(ctx->dec_ctx = avcodec_alloc_context3(dec))) {
    fprintf(stderr, "Failed to allocate decoder context.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret =
    avcodec_parameters_to_context(ctx->dec_ctx, in_stream->codecpar)) < 0)
  {
    fprintf(stderr, "Failed to copy codec parameters to decoder context.\n");
    goto end;
  }

  if ((ret = avcodec_open2(ctx->dec_ctx, dec, NULL)) < 0) {
    fprintf(stderr, "Failed to open decoder.\n");
    goto end;
  }

  if (!(ctx->dec_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

end:
  if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
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

int select_channel_layout(AVCodecContext *enc_ctx,
  AVChannelLayout *preferred_layout)
{
  const AVChannelLayout *layouts = NULL, *current_layout;
  char preferred_layout_name[64], current_layout_name[64];
  int preferred_nb_channels, ret = 0;

  if ((ret = avcodec_get_supported_config(enc_ctx, NULL,
    AV_CODEC_CONFIG_CHANNEL_LAYOUT, 0, (const void **) &layouts, NULL)) < 0)
  {
    fprintf(stderr, "Failed to get supported channel layouts.\n");
    return ret;
  }

  if ((ret = av_channel_layout_describe(preferred_layout,
    preferred_layout_name, sizeof(preferred_layout_name))) < 0)
  {
    fprintf(stderr, "Failed to get name for preferred layout.\n");
    return ret;
  }

  printf("preferred_layout: %s\n", preferred_layout_name);

  if (!layouts) {
    printf("No supported channel layouts list found. "
      "Attempting to set preferred layout.\n");

    if ((ret = av_channel_layout_copy(&enc_ctx->ch_layout,
      preferred_layout)) < 0)
    {
      fprintf(stderr, "Failed to copy preferred channel layout."
        "Attempting to set stereo channel layout\n");

      goto set_stereo;
    }
    
    printf("Channel layout set to %s.\n", preferred_layout_name);
    return 0;
  }

  printf("Checking if preferred layout is supported by encoder.\n");

  current_layout = layouts;

  while (current_layout->nb_channels) {
    if ((ret = av_channel_layout_describe(current_layout,
      current_layout_name, sizeof(current_layout_name))) < 0)
    {
      fprintf(stderr, "Failed to get name for current layout.\n");
      return ret;
    }

    printf("current_layout_name: %s\n", current_layout_name);

    if (!strcmp(current_layout_name, preferred_layout_name)) {
      if ((ret = av_channel_layout_copy(&enc_ctx->ch_layout, current_layout)) < 0) {
        fprintf(stderr, "Failed to copy preferred channel layout.\n");
        return ret;
      }

      printf("Channel layout set to preferred layout.\n");
      return 0;
    }

    current_layout++;
  }

  printf("Preferred layout not supported. Checking for supported layout "
    "with equivalent number of channels.\n");

  preferred_nb_channels = preferred_layout->nb_channels;
  current_layout = layouts;

  while (current_layout->nb_channels) {
    if ((ret = av_channel_layout_describe(current_layout, current_layout_name,
      sizeof(current_layout_name))) < 0)
    {
      fprintf(stderr, "Failed to get name of current layout.\n");
      return ret;
    }

    printf("current_layout_name: %s\n", current_layout_name);

    if (current_layout->nb_channels == preferred_nb_channels) {
      if ((ret =
        av_channel_layout_copy(&enc_ctx->ch_layout, current_layout)) < 0)
      {
        fprintf(stderr, "Failed to copy layout with preferred_nb_channels.\n");
        return ret;
      }

      printf("Channel layout set to %s.\n", current_layout_name);
      return 0;
    }

    current_layout++;
  }

  printf("No layout with equivalent number of channels supported. Attempting "
  "to set channel layout to stereo.\n");

set_stereo:
  if ((ret = av_channel_layout_copy(&enc_ctx->ch_layout,
    &(AVChannelLayout) AV_CHANNEL_LAYOUT_STEREO)) < 0)
  {
    fprintf(stderr, "Failed to copy stereo channel layout.\n");
    return ret;
  }

  printf("Channel layout set to stereo.\n");
  return 0;
}

int select_sample_fmt(AVCodecContext *enc_ctx, enum AVSampleFormat preferred_fmt)
{
  const enum AVSampleFormat *formats = NULL;
  const char *preferred_fmt_name, *current_fmt_name;
  int i = 0, ret = 0;

  if ((ret = avcodec_get_supported_config(enc_ctx, NULL,
    AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, (const void **) &formats, NULL)) < 0)
  {
    fprintf(stderr, "Failed to get supported sample formats.\n");
    return ret;
  }

  if (!(preferred_fmt_name = av_get_sample_fmt_name(preferred_fmt))) {
    fprintf(stderr, "Failed to get name of preferred_fmt.\n");
    return AVERROR_UNKNOWN;
  }

  printf("preferred format: %s\n", preferred_fmt_name);

  if (!formats) {
    printf("No supported sample formats list found. "
      "Setting sample format to preferred sample format.\n");

    enc_ctx->sample_fmt = preferred_fmt;
    return 0;
  }

  printf("Checking if preferred sample format is supported by encoder.\n");

  while (formats[i] && formats[i] != AV_SAMPLE_FMT_NONE)
  {
    if (!(current_fmt_name = av_get_sample_fmt_name(formats[i]))) {
      fprintf(stderr, "Failed to get name of current_fmt_name.\n");
      return AVERROR_UNKNOWN;
    }

    printf("current_fmt_name: %s\n", current_fmt_name);

    if (!strcmp(current_fmt_name, preferred_fmt_name))
    {
      enc_ctx->sample_fmt = formats[i];
      printf("Sample format set to preferred sample format.\n");
      return 0;
    }

    i++;
  }

  printf("Preferred sample format not supported. "
    "Setting sample format to first supported sample format.\n");

  if (!(current_fmt_name = av_get_sample_fmt_name(formats[0]))) {
    fprintf(stderr, "Failed to get name of first supported sample format.\n");
    return AVERROR_UNKNOWN;
  }

  printf("first supported sample format: %s\n", current_fmt_name);
  enc_ctx->sample_fmt = formats[0];

  return 0;
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
    goto end;
  }

  ctx->fmt_ctx = NULL;
  ctx->enc_ctx = NULL;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder.\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }

  if (!(ctx->enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder context.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = select_channel_layout(ctx->enc_ctx,
    &dec_ctx->ch_layout)) < 0)
  {
    fprintf(stderr, "Failed to select channel layout.\n");
    goto end;
  }

  if ((ret = select_sample_fmt(ctx->enc_ctx, dec_ctx->sample_fmt)) < 0) {
    fprintf(stderr, "Failed to select sample format.\n");
    goto end;
  }

  ctx->enc_ctx->sample_rate = dec_ctx->sample_rate;
  ctx->enc_ctx->time_base = (AVRational) {1, ctx->enc_ctx->sample_rate};
  ctx->enc_ctx->bit_rate = dec_ctx->bit_rate;

  if ((ret = avcodec_open2(ctx->enc_ctx, enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    goto end;
  }

  if ((ret = avformat_alloc_output_context2(&ctx->fmt_ctx,
    NULL, NULL, out_filename)))
  {
    fprintf(stderr, "Failed to allocate output format context.\n");
    goto end;
  }

  if ((ret = av_dict_copy(&ctx->fmt_ctx->metadata,
    fmt_metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy file metadata.\n");
    goto end;
  }

  if (!(out_stream = avformat_new_stream(ctx->fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate new output stream.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    stream_metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy audio metadata.\n");
    goto end;
  }

  if ((ret =
    avcodec_parameters_from_context(out_stream->codecpar, ctx->enc_ctx)))
  {
    fprintf(stderr,
      "Failed to copy codec parameters from encoder context to stream.\n");
      goto end;
  }

  out_stream->time_base = ctx->enc_ctx->time_base;

  if (!(ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if ((ret = avio_open(&ctx->fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0)
    {
      fprintf(stderr, "Failed to open output file.\n");
      goto end;
    }
  }

  if ((ret = avformat_write_header(ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to write header for output file.\n");
    goto end;
  }

end:
  if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
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

int initialize_swr(struct SwrContext **swr_ctx, AVCodecContext *dec_ctx,
  AVCodecContext *enc_ctx, AVFrame **swr_frame)
{
  int ret = 0;

  if (!(*swr_ctx = swr_alloc())) {
    fprintf(stderr, "Failed to allocate SwrContext.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  av_opt_set_chlayout(*swr_ctx, "in_chlayout", &dec_ctx->ch_layout, 0);
  av_opt_set_int(*swr_ctx, "in_sample_rate", dec_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(*swr_ctx, "in_sample_fmt", dec_ctx->sample_fmt, 0);
  av_opt_set_chlayout(*swr_ctx, "out_chlayout", &enc_ctx->ch_layout, 0);
  av_opt_set_int(*swr_ctx, "out_sample_rate", enc_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(*swr_ctx, "out_sample_fmt", enc_ctx->sample_fmt, 0);

  if ((ret = swr_init(*swr_ctx)) < 0) {
    fprintf(stderr, "Failed to initialize SwrContext.\n");
    return ret;
  }

  if (!(*swr_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  (*swr_frame)->format = enc_ctx->sample_fmt;
  av_channel_layout_copy(&(*swr_frame)->ch_layout, &enc_ctx->ch_layout);
  (*swr_frame)->sample_rate = enc_ctx->sample_rate;
  (*swr_frame)->nb_samples = 1536;
  printf("dec_ctx->frame_size: %d\n", dec_ctx->frame_size);

  if ((ret = av_frame_get_buffer(*swr_frame, 0)) < 0) {
    fprintf(stderr, "Failed to allocate buffers for frame.\n");
    return ret;
  }

  return 0;
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

  InputContext *input_ctx = NULL;
  OutputContext *output_ctx = NULL;

  struct SwrContext *swr_ctx = NULL;
  AVFrame *swr_frame = NULL;
  int nb_converted_samples;

  FrameSizeConversionContext *fsc_ctx = NULL;

  if (argc != 4) {
    printf("\nUsage: %s <input file> <output file> <codec>\n\n\t"
      "This example will take in a file with a video stream, transcode the video,\n\t"
      "and reformat the video into a new file.\n\n", argv[0]);
    return 0;
  }

  in_filename = argv[1];
  out_filename = argv[2];
  codec = argv[3];

  if (!(input_ctx = open_input(in_filename, 1))) {
    fprintf(stderr, "Failed to open input.\n");
    goto end;
  }

  if (!(output_ctx = open_output(input_ctx->dec_ctx, codec, out_filename,
    input_ctx->fmt_ctx->metadata,
    input_ctx->fmt_ctx->streams[input_ctx->stream_idx]->metadata)))
  {
    fprintf(stderr, "Failed to open output.\n");
  }

  if ((ret = initialize_swr(&swr_ctx, input_ctx->dec_ctx,
    output_ctx->enc_ctx, &swr_frame)) < 0) {
    fprintf(stderr, "Failed to initialize SwrContext.\n");
    goto end;
  }

  if (!(fsc_ctx = fsc_ctx_alloc(output_ctx->enc_ctx))) {
    fprintf(stderr, "Failed to initialize FrameSizeConversionContext.\n");
    ret = ENOMEM;
    goto end;
  }

  if (!(pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  while ((ret = av_read_frame(input_ctx->fmt_ctx, pkt)) >= 0)
  {
    if (!(pkt->stream_index == input_ctx->stream_idx)) {
      av_packet_unref(pkt);
      continue;
    }

    if ((ret = avcodec_send_packet(input_ctx->dec_ctx, pkt)) < 0) {
      fprintf(stderr, "Failed to send packet to decoder.\n");
      goto end;
    }

    while ((ret =
      avcodec_receive_frame(input_ctx->dec_ctx, input_ctx->dec_frame)) >= 0)
    {
      if ((ret = av_frame_make_writable(swr_frame)) < 0) {
        fprintf(stderr, "Failed to make frame writable.\n");
        goto end;
      }

      if ((ret = nb_converted_samples =
        swr_convert(swr_ctx, swr_frame->data, swr_frame->nb_samples,
        (const uint8_t **) input_ctx->dec_frame->data,
        input_ctx->dec_frame->nb_samples)) < 0)
      {
        fprintf(stderr, "Failed to convert audio frame.\n");
        goto end;
      }

      if ((ret = fsc_ctx_add_samples_to_buffer(
        fsc_ctx, swr_frame, nb_converted_samples)) < 0)
      {
        fprintf(stderr, "Failed to add samples to buffer.\n");
        goto end;
      }

      // printf("nb_converted_samples: %d\n", nb_converted_samples);
      // printf("swr_frame->nb_samples: %d\n", swr_frame->nb_samples);
      // printf("input_ctx->dec_frame->nb_samples: %d\n",
      //   input_ctx->dec_frame->nb_samples);

      while (fsc_ctx->nb_samples_in_buffer >= output_ctx->enc_ctx->frame_size)
      {
        if ((ret = fsc_ctx_make_frame(fsc_ctx)) < 0) {
          fprintf(stderr, "Failed to make frame for encoder.\n");
          goto end;
        }

        if ((ret =
          avcodec_send_frame(output_ctx->enc_ctx, fsc_ctx->frame)) < 0)
        {
          fprintf(stderr, "Failed to send frame to encoder.\n");
          goto end;
        }

        while ((ret = avcodec_receive_packet(output_ctx->enc_ctx, pkt)) >= 0)
        {
          pkt->stream_index = 0;

          if ((ret =
            av_interleaved_write_frame(output_ctx->fmt_ctx, pkt)) < 0)
          {
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

  if ((ret = avcodec_send_frame(output_ctx->enc_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to send frame to encoder.\n");
    goto end;
  }

  if ((ret = av_write_trailer(output_ctx->fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  close_input(input_ctx);
  close_output(output_ctx);
  swr_free(&swr_ctx);
  av_frame_free(&swr_frame);
  fsc_ctx_free(fsc_ctx);
  av_packet_free(&pkt);

  if (ret < 0 && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return -1;
  }
  return 0;
}
