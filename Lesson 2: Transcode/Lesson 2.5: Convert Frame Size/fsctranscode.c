#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

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

typedef struct OutputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *enc_ctx;
  AVPacket *enc_pkt;
} OutputContext;

OutputContext *open_output(InputContext *in_ctx,
  const char *codec, const char *out_filename)
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

  if (av_channel_layout_copy(&out_ctx->enc_ctx->ch_layout,
    &in_ctx->dec_ctx->ch_layout) < 0)
  {
    fprintf(stderr, "Failed to set channel layout on encoder.\n");
    return NULL;
  }

  out_ctx->enc_ctx->sample_fmt = in_ctx->dec_ctx->sample_fmt;
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
  AVFrame *dec_frame)
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
      "Failed to copy samples from decoded frame to sample_buffer.\n");
    return ret;
  }

  fsc_ctx->nb_samples_in_buffer += dec_frame->nb_samples;
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
  FrameSizeConversionContext *fsc_ctx, int flush)
{
  int ret = 0;

  if ((ret =
    fsc_ctx_add_samples_to_buffer(fsc_ctx, in_ctx->dec_frame)) < 0)
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
  FrameSizeConversionContext *fsc_ctx)
{
  int ret = 0;

  if ((ret = avcodec_send_packet(in_ctx->dec_ctx, in_ctx->init_pkt)) < 0) {
    fprintf(stderr, "Failed to send packet to decoder.\n");
    return ret;
  }

  while ((ret =
    avcodec_receive_frame(in_ctx->dec_ctx, in_ctx->dec_frame)) >= 0)
  {
    if ((ret = convert_frame_size(in_ctx, out_ctx, fsc_ctx, 0)) < 0) {
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
  FrameSizeConversionContext *fsc_ctx)
{
  int ret = 0;
  while ((ret = av_read_frame(in_ctx->fmt_ctx, in_ctx->init_pkt)) >= 0)
  {
    if (!(in_ctx->init_pkt->stream_index == in_ctx->stream_idx)) {
      av_packet_unref(in_ctx->init_pkt);
      continue;
    }

    if ((ret = decode_packet(in_ctx, out_ctx, fsc_ctx)) < 0) {
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
    codec, out_filename)))
  {
    fprintf(stderr, "Failed to open output.\n");
    goto end;
  }

  if (!(fsc_ctx = fsc_ctx_alloc(out_ctx->enc_ctx))) {
    fprintf(stderr, "Failed to initialize FrameSizeConversionContext.\n");
    ret = ENOMEM;
    goto end;
  }

  if ((ret = transcode(in_ctx, out_ctx, fsc_ctx)) < 0) {
    fprintf(stderr, "Failed to transcode file.\n");
    goto end;
  }

  in_ctx->init_pkt = NULL;
  if ((ret = decode_packet(in_ctx, out_ctx, fsc_ctx)) < 0) {
    fprintf(stderr, "Failed to flush decoder.\n");
    goto end;
  }

  if ((ret = convert_frame_size(in_ctx, out_ctx, fsc_ctx, 1)) < 0) {
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

  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return ret;
  }

  return 0;
}
