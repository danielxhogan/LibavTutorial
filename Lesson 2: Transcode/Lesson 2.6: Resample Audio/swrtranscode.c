#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>

typedef struct InputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *dec_ctx;
  AVPacket *init_pkt;
  AVFrame *dec_frame;
  int stream_idx;
} InputContext;

InputContext *open_input(const char *in_filename, int stream_idx)
{
  InputContext *in_ctx;
  AVStream *in_stream;
  const AVCodec *dec;

  if (!(in_ctx = malloc(sizeof(InputContext)))) {
    fprintf(stderr, "Failed to allocate input context.\n");
    return NULL;
  }

  in_ctx->fmt_ctx = NULL;
  in_ctx->dec_ctx = NULL;
  in_ctx->init_pkt = NULL;
  in_ctx->dec_frame = NULL;
  in_ctx->stream_idx = stream_idx;

  if (avformat_open_input(&in_ctx->fmt_ctx, in_filename, NULL, NULL) < 0) {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", in_filename);
    return NULL;
  }

  if (avformat_find_stream_info(in_ctx->fmt_ctx, NULL) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    return NULL;
  }

  in_stream = in_ctx->fmt_ctx->streams[stream_idx];

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder.\n");
    return NULL;
  }

  if (!(in_ctx->dec_ctx = avcodec_alloc_context3(dec))) {
    fprintf(stderr, "Failed to allocate decoder context.\n");
    return NULL;
  }

  if (avcodec_parameters_to_context(in_ctx->dec_ctx, in_stream->codecpar) < 0) {
    fprintf(stderr, "Failed to copy codec parameters to decoder context.\n");
    return NULL;
  }

  if (avcodec_open2(in_ctx->dec_ctx, dec, NULL) < 0) {
    fprintf(stderr, "Failed to open decoder.\n");
    return NULL;
  }

  if (!(in_ctx->dec_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    return NULL;
  }

  if (!(in_ctx->init_pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    return NULL;
  }

  return in_ctx;
}

void close_input(InputContext *in_ctx)
{
  if (!in_ctx) return;
  avformat_close_input(&in_ctx->fmt_ctx);
  avcodec_free_context(&in_ctx->dec_ctx);
  av_packet_free(&in_ctx->init_pkt);
  av_frame_free(&in_ctx->dec_frame);
  free(in_ctx);
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
  AVPacket *enc_pkt;
} OutputContext;

OutputContext *open_output(InputContext *in_ctx, const char *codec,
  const char *out_filename, AVChannelLayout *preferred_layout)
{
  OutputContext *out_ctx;
  AVStream *in_stream, *out_stream;
  const AVCodec *enc;

  if (!( out_ctx = malloc(sizeof(OutputContext)))) {
    fprintf(stderr, "Failed to allocate output context.\n");
    return NULL;
  }

  out_ctx->fmt_ctx = NULL;
  out_ctx->enc_ctx = NULL;
  out_ctx->enc_pkt = NULL;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder.\n");
    return NULL;
  }

  if (!(out_ctx->enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder context.\n");
    return NULL;
  }

  if (preferred_layout) {
    if (select_channel_layout(out_ctx->enc_ctx, preferred_layout) < 0) {
      fprintf(stderr, "Failed to select channel layout.\n");
      return NULL;
    }
  } else {
    if (select_channel_layout(out_ctx->enc_ctx, &in_ctx->dec_ctx->ch_layout) < 0) {
      fprintf(stderr, "Failed to select channel layout.\n");
      return NULL;
    }
  }

  if (select_sample_fmt(out_ctx->enc_ctx, in_ctx->dec_ctx->sample_fmt) < 0) {
    fprintf(stderr, "Failed to select sample format.\n");
    return NULL;
  }

  out_ctx->enc_ctx->sample_rate = in_ctx->dec_ctx->sample_rate;
  out_ctx->enc_ctx->time_base = (AVRational) {1, out_ctx->enc_ctx->sample_rate};

  if (in_ctx->dec_ctx->bit_rate > 0)
    out_ctx->enc_ctx->bit_rate = in_ctx->dec_ctx->bit_rate;
  else
    out_ctx->enc_ctx->bit_rate = 224000;

  if (avcodec_open2(out_ctx->enc_ctx, enc, NULL) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    return NULL;
  }

  if (avformat_alloc_output_context2(&out_ctx->fmt_ctx,
    NULL, NULL, out_filename))
  {
    fprintf(stderr, "Failed to allocate output format context.\n");
    return NULL;
  }

  if (av_dict_copy(&out_ctx->fmt_ctx->metadata,
    in_ctx->fmt_ctx->metadata, AV_DICT_DONT_OVERWRITE) < 0)
  {
    fprintf(stderr, "Failed to copy file metadata.\n");
    return NULL;
  }

  if (!(out_stream = avformat_new_stream(out_ctx->fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate new output stream.\n");
    return NULL;
  }

  in_stream = in_ctx->fmt_ctx->streams[in_ctx->stream_idx];

  if (av_dict_copy(&out_stream->metadata,
    in_stream->metadata, AV_DICT_DONT_OVERWRITE) < 0)
  {
    fprintf(stderr, "Failed to copy audio metadata.\n");
    return NULL;
  }

  if (avcodec_parameters_from_context(out_stream->codecpar, out_ctx->enc_ctx))
  {
    fprintf(stderr,
      "Failed to copy codec parameters from encoder context to stream.\n");
      return NULL;
  }

  if (!(out_ctx->enc_pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    return NULL;
  }

  out_stream->time_base = out_ctx->enc_ctx->time_base;

  if (!(out_ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&out_ctx->fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
      fprintf(stderr, "Failed to open output file.\n");
      return NULL;
    }
  }

  if (avformat_write_header(out_ctx->fmt_ctx, NULL) < 0) {
    fprintf(stderr, "Failed to write header for output file.\n");
    return NULL;
  }

  return out_ctx;
}

void close_output(OutputContext *out_ctx)
{
  if (!out_ctx) return;
  if (out_ctx->fmt_ctx && !(out_ctx->fmt_ctx->flags & AVFMT_NOFILE))
    avio_closep(&out_ctx->fmt_ctx->pb);
  avformat_free_context(out_ctx->fmt_ctx);
  avcodec_free_context(&out_ctx->enc_ctx);
  av_packet_free(&out_ctx->enc_pkt);
}

typedef struct SwrOutputContext {
  struct SwrContext *swr_ctx;
  AVFrame *swr_frame;
  int nb_converted_samples;
} SwrOutputContext;

SwrOutputContext *swr_output_context_alloc(InputContext *in_ctx,
  OutputContext *out_ctx)
{
  SwrOutputContext *swr_out_ctx;

  if (!(swr_out_ctx = malloc(sizeof(SwrOutputContext)))) {
    fprintf(stderr, "Failed to allocate swr output context.\n");
    return NULL;
  }

  swr_out_ctx->swr_ctx = NULL;
  swr_out_ctx->swr_frame = NULL;
  swr_out_ctx->nb_converted_samples = 0;

  if (!(swr_out_ctx->swr_ctx = swr_alloc())) {
    fprintf(stderr, "Failed to allocate SwrContext.\n");
    return NULL;
  }

  av_opt_set_chlayout(swr_out_ctx->swr_ctx,
    "in_chlayout", &in_ctx->dec_ctx->ch_layout, 0);
  av_opt_set_int(swr_out_ctx->swr_ctx,
    "in_sample_rate", in_ctx->dec_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(swr_out_ctx->swr_ctx,
    "in_sample_fmt", in_ctx->dec_ctx->sample_fmt, 0);
  av_opt_set_chlayout(swr_out_ctx->swr_ctx,
    "out_chlayout", &out_ctx->enc_ctx->ch_layout, 0);
  av_opt_set_int(swr_out_ctx->swr_ctx,
    "out_sample_rate", out_ctx->enc_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(swr_out_ctx->swr_ctx,
    "out_sample_fmt", out_ctx->enc_ctx->sample_fmt, 0);

  if (swr_init(swr_out_ctx->swr_ctx) < 0) {
    fprintf(stderr, "Failed to initialize SwrContext.\n");
    return NULL;
  }

  if (!(swr_out_ctx->swr_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    return NULL;
  }

  swr_out_ctx->swr_frame->format = out_ctx->enc_ctx->sample_fmt;
  av_channel_layout_copy(&swr_out_ctx->swr_frame->ch_layout,
    &out_ctx->enc_ctx->ch_layout);
  swr_out_ctx->swr_frame->sample_rate = out_ctx->enc_ctx->sample_rate;
  swr_out_ctx->swr_frame->nb_samples = 1536;

  if (av_frame_get_buffer(swr_out_ctx->swr_frame, 0) < 0) {
    fprintf(stderr, "Failed to allocate buffers for frame.\n");
    return NULL;
  }

  return swr_out_ctx;
}

void swr_output_context_free(SwrOutputContext *swr_out_ctx)
{
  if (!swr_out_ctx) return;
  swr_free(&swr_out_ctx->swr_ctx);
  av_frame_free(&swr_out_ctx->swr_frame);
  free(swr_out_ctx);
}

#define SAMPLE_BUFFER_LENGTH AV_NUM_DATA_POINTERS

typedef struct FrameSizeConversionContext {
  uint8_t *sample_buffer[SAMPLE_BUFFER_LENGTH];
  int sample_buffer_capacity;
  int channels;
  int bytes_per_sample;
  enum AVSampleFormat sample_fmt;
  int nb_samples_in_buffer;

  AVFrame *frame;
  int frame_size;
  int64_t nb_samples_framed;
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
  fsc_ctx->channels = enc_ctx->ch_layout.nb_channels;
  fsc_ctx->bytes_per_sample =
    av_get_bytes_per_sample(enc_ctx->sample_fmt) * fsc_ctx->channels;
  fsc_ctx->sample_fmt = enc_ctx->sample_fmt;
  fsc_ctx->nb_samples_in_buffer = 0;

  if (!(fsc_ctx->frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    return NULL;
  }

  fsc_ctx->frame_size = enc_ctx->frame_size;
  fsc_ctx->nb_samples_framed = 0;

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
  AVFrame *dec_frame, int nb_samples)
{
  int ret = 0;

  if ((ret = fsc_ctx_alloc_buffer(fsc_ctx,
    fsc_ctx->frame_size + nb_samples)) < 0)
  {
    fprintf(stderr, "Failed to allocate sample buffer for fsc_ctx.\n");
    return ret;
  }

  ret = av_samples_copy(fsc_ctx->sample_buffer, dec_frame->data,
    fsc_ctx->nb_samples_in_buffer, 0, nb_samples,
    fsc_ctx->channels, fsc_ctx->sample_fmt);

  if (ret < 0) {
    fprintf(stderr,
      "Failed to copy samples from decoded frame to sample_buffer.\n");
    return ret;
  }

  fsc_ctx->nb_samples_in_buffer += nb_samples;
  return ret;
}

int fsc_ctx_make_frame(FrameSizeConversionContext *fsc_ctx, int flush)
{
  int nb_remaining_samples, ret = 0;
  uint8_t *new_first_sample;

  if (!flush && fsc_ctx->nb_samples_in_buffer < fsc_ctx->frame_size) {
    fprintf(stderr, "Not enough samples in buffer to make a frame.\n");
    return -1;
  }

  if ((ret = av_samples_copy(fsc_ctx->frame->data, fsc_ctx->sample_buffer, 0, 0,
    fsc_ctx->frame_size, fsc_ctx->channels, fsc_ctx->sample_fmt)) < 0)
  {
    fprintf(stderr, "Failed to copy samples from buffer into encoder frame.\n");
    return ret;
    }

  fsc_ctx->frame->pts = fsc_ctx->nb_samples_framed;
  fsc_ctx->nb_samples_framed += fsc_ctx->frame->nb_samples;

  new_first_sample =
    fsc_ctx->sample_buffer[0] +
    fsc_ctx->frame_size *
    fsc_ctx->bytes_per_sample;

  nb_remaining_samples =
    (fsc_ctx->nb_samples_in_buffer - fsc_ctx->frame_size) *
    fsc_ctx->bytes_per_sample;

  memmove(fsc_ctx->sample_buffer[0], new_first_sample, nb_remaining_samples);

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

int encode_frame(InputContext *in_ctx, OutputContext *out_ctx,
  FrameSizeConversionContext *fsc_ctx)
{
  int ret = 0;

  if ((ret = avcodec_send_frame(out_ctx->enc_ctx, fsc_ctx->frame)) < 0)
  {
    fprintf(stderr, "Failed to send frame to encoder.\n");
    return ret;
  }

  while ((ret =
    avcodec_receive_packet(out_ctx->enc_ctx, out_ctx->enc_pkt)) >= 0)
  {
    out_ctx->enc_pkt->stream_index = 0;

    av_packet_rescale_ts(out_ctx->enc_pkt,
      out_ctx->enc_ctx->time_base, out_ctx->fmt_ctx->streams[0]->time_base);

    if ((ret =
      av_interleaved_write_frame(out_ctx->fmt_ctx, out_ctx->enc_pkt)) < 0)
    {
      fprintf(stderr, "Failed to write packet to file.\n");
      return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to receive packet from encoder.\n");
    return ret;
  }

  return 0;
}

int convert_frame_size(InputContext *in_ctx, OutputContext *out_ctx,
  FrameSizeConversionContext *fsc_ctx, SwrOutputContext *swr_out_ctx, int flush)
{
  int ret = 0;

  if ((ret =
    fsc_ctx_add_samples_to_buffer(fsc_ctx, swr_out_ctx->swr_frame,
      swr_out_ctx->nb_converted_samples)) < 0)
  {
    fprintf(stderr, "Failed to add samples to buffer.\n");
    return ret;
  }

  while (fsc_ctx->nb_samples_in_buffer >= out_ctx->enc_ctx->frame_size)
  {
    if ((ret = fsc_ctx_make_frame(fsc_ctx, flush)) < 0) {
      fprintf(stderr, "Failed to make frame for encoder.\n");
      return ret;
    }

    if ((ret = encode_frame(in_ctx, out_ctx, fsc_ctx)) < 0) {
      fprintf(stderr, "Failed to encode frame.\n");
      return ret;
    }
  }

  return 0;
}

int decode_packet(InputContext *in_ctx, OutputContext *out_ctx,
  FrameSizeConversionContext *fsc_ctx, SwrOutputContext *swr_out_ctx)
{
  int ret = 0;

  if ((ret = avcodec_send_packet(in_ctx->dec_ctx, in_ctx->init_pkt)) < 0) {
    fprintf(stderr, "Failed to send packet to decoder.\n");
    return ret;
  }

  while ((ret =
    avcodec_receive_frame(in_ctx->dec_ctx, in_ctx->dec_frame)) >= 0)
  {
    if ((ret = av_frame_make_writable(swr_out_ctx->swr_frame)) < 0) {
      fprintf(stderr, "Failed to make frame writable.\n");
      return ret;
    }

    if ((ret = swr_out_ctx->nb_converted_samples =
      swr_convert(swr_out_ctx->swr_ctx, swr_out_ctx->swr_frame->data,
        swr_out_ctx->swr_frame->nb_samples,
      (const uint8_t **) in_ctx->dec_frame->data,
      in_ctx->dec_frame->nb_samples)) < 0)
    {
      fprintf(stderr, "Failed to convert audio frame.\n");
      return ret;
    }

    if ((ret = convert_frame_size(in_ctx, out_ctx,
      fsc_ctx, swr_out_ctx, 0)) < 0)
    {
      fprintf(stderr, "Failed to convert frame size.\n");
      return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to receive frame from decoder.\n");
    return ret;
  }

  return 0;
}

int transcode(InputContext *in_ctx, OutputContext *out_ctx,
  FrameSizeConversionContext *fsc_ctx, SwrOutputContext *swr_out_ctx)
{
  int ret = 0;
  while ((ret = av_read_frame(in_ctx->fmt_ctx, in_ctx->init_pkt)) >= 0)
  {
    if (!(in_ctx->init_pkt->stream_index == in_ctx->stream_idx)) {
      av_packet_unref(in_ctx->init_pkt);
      continue;
    }

    if ((ret = decode_packet(in_ctx, out_ctx, fsc_ctx, swr_out_ctx)) < 0) {
      fprintf(stderr, "Failed to decode packet.\n");
      return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to read frame.\n");
    return ret;
  }

  return 0;
}

int main(int argc, char **argv)
{
  const char *in_filename, *out_filename, *codec;
  int ret = 0;

  InputContext *in_ctx = NULL;
  OutputContext *out_ctx = NULL;

  FrameSizeConversionContext *fsc_ctx = NULL;
  SwrOutputContext *swr_out_ctx = NULL;
  AVChannelLayout *stereo = NULL;

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

  if (!(out_ctx = open_output(in_ctx,
    codec, out_filename, NULL)))
  {
    fprintf(stderr, "Failed to open output.\n");
    close_output(out_ctx);

    stereo = malloc(sizeof(AVChannelLayout));
    av_channel_layout_from_string(stereo, "stereo");

    if (!(out_ctx = open_output(in_ctx,
      codec, out_filename, stereo)))
    {
      fprintf(stderr, "Failed to open output with stereo layout.\n");
      goto end;
    }
  }

  if (!(fsc_ctx = fsc_ctx_alloc(out_ctx->enc_ctx))) {
    fprintf(stderr, "Failed to initialize FrameSizeConversionContext.\n");
    ret = ENOMEM;
    goto end;
  }

  if (!(swr_out_ctx = swr_output_context_alloc(in_ctx, out_ctx))) {
    fprintf(stderr, "Failed to allocate swr output context.\n");
    goto end;
  }

  if ((ret = transcode(in_ctx, out_ctx, fsc_ctx, swr_out_ctx)) < 0) {
    fprintf(stderr, "Failed to transcode file.\n");
    goto end;
  }

  in_ctx->init_pkt = NULL;
  if ((ret = decode_packet(in_ctx, out_ctx, fsc_ctx, swr_out_ctx)) < 0) {
    fprintf(stderr, "Failed to flush decoder.\n");
    goto end;
  }

  if ((ret = convert_frame_size(in_ctx, out_ctx, fsc_ctx, swr_out_ctx, 1)) < 0) {
    fprintf(stderr, "Failed to flush frame size converter.\n");
    goto end;
  }

  fsc_ctx->frame = NULL;
  if ((ret = encode_frame(in_ctx, out_ctx, fsc_ctx)) < 0) {
    fprintf(stderr, "Failed to flush encoder.\n");
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
  swr_output_context_free(swr_out_ctx);
  av_channel_layout_uninit(stereo);
  free(stereo);

  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return ret;
  }

  return 0;
}
