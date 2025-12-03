#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>

#define OUTPUT_SCALE_ALGO SWS_BICUBIC

typedef struct SwsOutputContext {
  struct SwsContext *sws_ctx;
  int width;
  int height;
  enum AVPixelFormat pix_fmt;
  int scale_algo;
  AVFrame *frame;
} SwsOutputContext;

SwsOutputContext *sws_output_context_alloc(int width, int height,
  enum AVPixelFormat pix_fmt, int scale_algo, AVCodecParameters *input_params)
{
  SwsOutputContext *sws_out_ctx;

  if (!(sws_out_ctx = malloc(sizeof(SwsOutputContext)))) {
    fprintf(stderr, "Failed to allocate SwsOutputContext.\n");
    return NULL;
  }

  sws_out_ctx->width = width;
  sws_out_ctx->height = height;
  sws_out_ctx->pix_fmt = pix_fmt;
  sws_out_ctx->scale_algo = scale_algo;
  sws_out_ctx->frame = NULL;

  if (!(sws_out_ctx->sws_ctx = sws_getContext(
    input_params->width,
    input_params->height,
    input_params->format,
    width, height, pix_fmt, scale_algo,
    NULL, NULL, NULL)))
  {
    fprintf(stderr, "Failed to get SwsContext.\n");
    return NULL;
  }

  if (!(sws_out_ctx->frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate sws_out_ctx->frame.\n");
    return NULL;
  }

  sws_out_ctx->frame->width = width;
  sws_out_ctx->frame->height = height;
  sws_out_ctx->frame->format = pix_fmt;

  if (av_frame_get_buffer(sws_out_ctx->frame, 0) < 0) {
    fprintf(stderr, "Failed to allocate buffers for frame.\n");
    return NULL;
  }

  return sws_out_ctx;
}

int sws_output_context_scale(SwsOutputContext *sws_out_ctx, AVFrame *frame)
{
  int ret = 0;

  if ((ret = av_frame_make_writable(sws_out_ctx->frame)) < 0) {
    fprintf(stderr, "Failed to make frame writable.\n");
    return ret;
  }

  ret = sws_scale(sws_out_ctx->sws_ctx, (const uint8_t * const *) frame->data,
    frame->linesize, 0, frame->height, sws_out_ctx->frame->data,
    sws_out_ctx->frame->linesize);

  sws_out_ctx->frame->pts = frame->pts;
  sws_out_ctx->frame->pkt_dts = frame->pkt_dts;

  return ret;
}

void sws_output_context_free(SwsOutputContext *sws_out_ctx)
{
  if (sws_out_ctx == NULL) return;
  sws_freeContext(sws_out_ctx->sws_ctx);
  av_frame_free(&sws_out_ctx->frame);
  free(sws_out_ctx);
}

typedef struct InputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *dec_ctx;
  AVFrame *dec_frame;
  AVPacket *init_pkt;
  int stream_idx;
} InputContext;

InputContext *open_input(const char *in_filename, unsigned int stream_idx)
{
  int ret = 0;
  InputContext *in_ctx = NULL;
  AVStream *in_stream;
  const AVCodec *dec;

  if (!(in_ctx = malloc(sizeof(InputContext)))) {
    fprintf(stderr,
      "Failed to allocate InputContext\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  in_ctx->fmt_ctx = NULL;
  in_ctx->dec_ctx = NULL;
  in_ctx->dec_frame = NULL;
  in_ctx->init_pkt = NULL;
  in_ctx->stream_idx = stream_idx;

  if ((ret =
    avformat_open_input(&in_ctx->fmt_ctx, in_filename, NULL, NULL)) < 0)
  {
    fprintf(stderr, "Failed to open AVFormatContext\n");
    goto end;
  }

  if ((ret = avformat_find_stream_info(in_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to find stream info\n");
    goto end;
  }

  if (stream_idx >= in_ctx->fmt_ctx->nb_streams) {
    fprintf(stderr, "Invalid stream index\n");
    ret = -1;
    goto end;
  }

  in_stream = in_ctx->fmt_ctx->streams[stream_idx];

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder for stream: '%d'\n", stream_idx);
    ret = AVERROR(EINVAL);
    goto end;
  }

  if (!(in_ctx->dec_ctx = avcodec_alloc_context3(dec))) {
    fprintf(stderr,
      "Failed to allocate decoder for stream: '%d'\n", stream_idx);
    ret = AVERROR(EINVAL);
    goto end;
  }

  if ((ret =
    avcodec_parameters_to_context(in_ctx->dec_ctx, in_stream->codecpar)) < 0)
  {
    fprintf(stderr,
      "Failed to copy parameters from input stream: '%d' to decoder\n",
      stream_idx);
    goto end;
  }

  if ((ret = avcodec_open2(in_ctx->dec_ctx, dec, NULL)) < 0) {
    fprintf(stderr, "Failed to open decoder for stream: '%d'\n",
      stream_idx);
    goto end;
  }

  if (!(in_ctx->dec_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(in_ctx->init_pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

end:
  if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return NULL;
  }

  return in_ctx;
}

void close_input(InputContext *in_ctx)
{
  if (!in_ctx) return;
  avformat_close_input(&in_ctx->fmt_ctx);
  avcodec_free_context(&in_ctx->dec_ctx);
  av_frame_free(&in_ctx->dec_frame);
  av_packet_free(&in_ctx->init_pkt);
  free(in_ctx);
}

int initialize_encoder_params(const char *encoder, char **enc_params_opt)
{
  if (strcmp(encoder, "libx264") == 0) {
    *enc_params_opt = "x264-params";
  }
  else if (strcmp(encoder, "libx265") == 0) {
    *enc_params_opt = "x265-params";
  }
  else if (strcmp(encoder, "libsvtav1") == 0) {
    *enc_params_opt = "svtav1-params";
  }
  else {
    fprintf(stderr, "Encoder not supported.\n");
    return -1;
  }
  return 0;
}

typedef struct OutputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *enc_ctx;
  AVPacket *enc_pkt;
} OutputContext;

OutputContext *open_output(InputContext *in_ctx,
  const char *codec, char *enc_params, char *enc_params_opt,
  int width, int height, enum AVPixelFormat pix_fmt,
  const char *out_filename)
{
  printf("width: %d\n", width);
  int ret = 0;
  OutputContext *out_ctx = NULL;
  AVStream *in_stream, *out_stream;
  const AVCodec *enc;

  if (!(out_ctx = malloc(sizeof(OutputContext)))) {
    fprintf(stderr,
      "Failed to allocate OutputContext\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  out_ctx->fmt_ctx = NULL;
  out_ctx->enc_ctx = NULL;
  out_ctx->enc_pkt = NULL;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder\n");
    ret = AVERROR(EINVAL);
    goto end;
  }

  if (!(out_ctx->enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder\n");
    ret = AVERROR(EINVAL);
    goto end;
  }

  in_stream = in_ctx->fmt_ctx->streams[in_ctx->stream_idx];

  out_ctx->enc_ctx->time_base = in_stream->time_base;
  out_ctx->enc_ctx->framerate = in_stream->avg_frame_rate;

  out_ctx->enc_ctx->width = width;
  out_ctx->enc_ctx->height = height;
  out_ctx->enc_ctx->pix_fmt = pix_fmt;

  out_ctx->enc_ctx->color_primaries = in_stream->codecpar->color_primaries;
  out_ctx->enc_ctx->color_trc = in_stream->codecpar->color_trc;
  out_ctx->enc_ctx->colorspace = in_stream->codecpar->color_space;
  out_ctx->enc_ctx->color_range = in_stream->codecpar->color_range;
  out_ctx->enc_ctx->chroma_sample_location = in_stream->codecpar->chroma_location;

  out_ctx->enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (enc_params && enc_params_opt) {
    if ((ret = av_opt_set(out_ctx->enc_ctx->priv_data,
      enc_params_opt, enc_params, 0)) < 0)
    {
      fprintf(stderr, "Failed to set %s\n", enc_params_opt);
      goto end;
    }
  }

  if ((ret = avcodec_open2(out_ctx->enc_ctx, enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder\n");
    goto end;
  }

  if ((ret =
    avformat_alloc_output_context2(&out_ctx->fmt_ctx, NULL, NULL, out_filename)))
  {
    fprintf(stderr,
      "Failed to allocate output format context\n");
    goto end;
  }

  if ((ret = av_dict_copy(&out_ctx->fmt_ctx->metadata, in_ctx->fmt_ctx->metadata,
    AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr,
      "Failed to copy input metadata to output\n");
    goto end;
  }

  if (!(out_stream = avformat_new_stream(out_ctx->fmt_ctx, NULL))) {
    fprintf(stderr,
      "Failed to allocate new output stream\n");
    goto end;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    in_stream->metadata, AV_DICT_DONT_OVERWRITE)))
  {
    fprintf(stderr,
      "Failed to copy metadata from input stream to output stream\n");
    goto end;
  }

  if ((ret =
    avcodec_parameters_from_context(out_stream->codecpar, out_ctx->enc_ctx)))
  {
    fprintf(stderr,
      "Failed to copy parameters from encoder to output stream\n");
    goto end;
  }

  out_stream->time_base = out_ctx->enc_ctx->time_base;
  out_stream->r_frame_rate = in_stream->r_frame_rate;
  out_stream->avg_frame_rate = in_stream->avg_frame_rate;

  if (!(out_ctx->enc_pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(out_ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if ((ret =
      avio_open(&out_ctx->fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0)
    {
      fprintf(stderr, "Failed to create output file\n");
      goto end;
    }
  }

  if ((ret = avformat_write_header(out_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to write header to output\n");
    goto end;
  }

end:
  if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
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
  free(out_ctx);
}

int transcode(InputContext *in_ctx, AVStream *in_stream,
  OutputContext *out_ctx, AVStream *out_stream,
  SwsOutputContext *sws_out_ctx)
{
  int ret = 0;

  while ((ret = av_read_frame(in_ctx->fmt_ctx, in_ctx->init_pkt)) >= 0)
  {
    if (!(in_ctx->init_pkt->stream_index == in_ctx->stream_idx)) {
      av_packet_unref(in_ctx->init_pkt);
      continue;
    }

    if ((ret = avcodec_send_packet(in_ctx->dec_ctx, in_ctx->init_pkt)) < 0) {
      fprintf(stderr, "Failed to send packet to decoder.\n");
      return ret;
    }

    while ((ret = avcodec_receive_frame(in_ctx->dec_ctx, in_ctx->dec_frame)) >= 0)
    {
      if ((ret = sws_output_context_scale(sws_out_ctx, in_ctx->dec_frame)) < 0) {
        fprintf(stderr, "Failed to scale frame.\n");
        return ret;
      }

      if ((ret = avcodec_send_frame(out_ctx->enc_ctx, sws_out_ctx->frame)) < 0) {
        fprintf(stderr, "Failed to send frame to encoder.\n");
        return ret;
      }

      while ((ret =
        avcodec_receive_packet(out_ctx->enc_ctx, out_ctx->enc_pkt)) >= 0)
      {
        out_ctx->enc_pkt->stream_index = 0;

        av_packet_rescale_ts(out_ctx->enc_pkt,
          in_stream->time_base, out_stream->time_base);

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
    }

    if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
      fprintf(stderr, "Failed to receive frame from decoder.\n");
      return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to read frame from input file.\n");
    return ret;
  }

  return 0;
}

int flush_decoder(InputContext *in_ctx, AVStream *in_stream,
  OutputContext *out_ctx, AVStream *out_stream)
{
  int ret = 0;

  if ((ret = avcodec_send_packet(in_ctx->dec_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to send packet to decoder.\n");
    return ret;
  }

  while ((ret = avcodec_receive_frame(in_ctx->dec_ctx, in_ctx->dec_frame)) >= 0)
  {
    if ((ret = avcodec_send_frame(out_ctx->enc_ctx, in_ctx->dec_frame)) < 0) {
      fprintf(stderr, "Failed to send frame to encoder.\n");
      return ret;
    }

    while ((ret =
      avcodec_receive_packet(out_ctx->enc_ctx, out_ctx->enc_pkt)) >= 0)
    {
      out_ctx->enc_pkt->stream_index = 0;

      av_packet_rescale_ts(out_ctx->enc_pkt,
        in_stream->time_base, out_stream->time_base);

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
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to receive frame from decoder.\n");
    return ret;
  }

  return 0;
}

int flush_encoder(OutputContext *out_ctx, AVStream *out_stream,
  AVStream *in_stream)
{
  int ret = 0;

  if ((ret = avcodec_send_frame(out_ctx->enc_ctx, NULL)) < 0)
  {
    fprintf(stderr, "Failed to send frame to encoder.\n");
    return ret;
  }

  while ((ret =
    avcodec_receive_packet(out_ctx->enc_ctx, out_ctx->enc_pkt)) >= 0)
  {
    out_ctx->enc_pkt->stream_index = 0;

    av_packet_rescale_ts(out_ctx->enc_pkt,
      in_stream->time_base, out_stream->time_base);

    if ((ret =
      av_interleaved_write_frame(out_ctx->fmt_ctx, out_ctx->enc_pkt)) < 0)
    {
      fprintf(stderr, "Failed to write packet to file.\n");
      return 0;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to receive packet from encoder.\n");
    return ret;
  }

  return 0;
}

int main(int argc, char **argv)
{
  int ret = 0;
  char *in_filename, *out_filename,
    *width_str, *height_str, *pix_fmt_str,
    *codec, *enc_params = NULL, *enc_params_opt = NULL;
  int width, height;
  enum AVPixelFormat pix_fmt;
  InputContext *in_ctx = NULL;
  OutputContext *out_ctx = NULL;
  SwsOutputContext *sws_out_ctx = NULL;
  AVStream *in_stream, *out_stream;

  if (argc != 7 && argc != 8) {
    printf("\nUsage: %s <input file> <output file> <encoder> [<encoder-params>]\n\n\t"
      "This example will take in a file with a video stream,\n\t"
      "transcode the video, and save it to <output file>.\n\t"
      "Optionally, you can pass in a colon seperated string\n\t"
      "with parameters that will be passed to the encoder.\n\t"
      "encoder-params is supported for libx264, libx265, and libsvtav1.\n\n",
      argv[0]);
    return 0;
  }

  in_filename = argv[1];
  out_filename = argv[2];
  width_str = argv[3];
  height_str = argv[4];
  pix_fmt_str = argv[5];
  codec = argv[6];

  if (!(width = atoi(width_str))) {
    fprintf(stderr, "Invalid value entered for width.\n");
    goto end;
  }

  if (!(height = atoi(height_str))) {
    fprintf(stderr, "Invalid value entered for height.\n");
    goto end;
  }

  if ((pix_fmt = av_get_pix_fmt(pix_fmt_str)) == AV_PIX_FMT_NONE) {
    fprintf(stderr, "Invalid value entered for pix_fmt.\n");
    goto end;
  }

  if (argc == 8) {
    enc_params = argv[7];

    if ((ret = initialize_encoder_params(codec, &enc_params_opt)) < 0) {
      fprintf(stderr, "Failed to initialize encoder option params\n");
      return -1;
    }
  }

  if (!(in_ctx = open_input(in_filename, 0))) {
    fprintf(stderr, "Failed to open input file: '%s'\n", in_filename);
    goto end;
  }

  in_stream = in_ctx->fmt_ctx->streams[in_ctx->stream_idx];

  if (!(out_ctx =
    open_output(in_ctx, codec, enc_params, enc_params_opt,
      width, height, pix_fmt, out_filename)))
  {
    fprintf(stderr, "Failed to open output file: '%s'\n", out_filename);
    goto end;
  }

  out_stream = out_ctx->fmt_ctx->streams[0];

  if (!(sws_out_ctx = sws_output_context_alloc(width, height,
    pix_fmt, OUTPUT_SCALE_ALGO, in_stream->codecpar)))
  {
    fprintf(stderr, "Failed to allocate SwsOutputContext.\n");
    ret = ENOMEM;
    goto end;
  }

  if ((ret = transcode(in_ctx, in_stream, out_ctx, out_stream, sws_out_ctx)) < 0) {
    fprintf(stderr, "Failed to transcode\n");
    goto end;
  }

  if ((ret = flush_decoder(in_ctx, in_stream, out_ctx, out_stream)) < 0) {
    fprintf(stderr, "Failed to flush decoder\n");
    goto end;
  }

  if ((ret = flush_encoder(out_ctx, out_stream, in_stream)) < 0) {
    fprintf(stderr, "Failed to flush encoder\n");
    goto end;
  }

  if ((ret = av_write_trailer(out_ctx->fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  close_input(in_ctx);
  close_output(out_ctx);
  sws_output_context_free(sws_out_ctx);

  if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return ret;
  }

  return 0;
}
