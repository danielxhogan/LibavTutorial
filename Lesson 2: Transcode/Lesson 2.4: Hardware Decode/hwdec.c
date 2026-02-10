#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>

#define OUTPUT_SCALE_ALGO SWS_BICUBIC

typedef struct SwsOutputContext {
  struct SwsContext *sws_ctx;
  AVFrame *scaled_frame;
} SwsOutputContext;

SwsOutputContext *sws_output_context_alloc(AVCodecContext *dec_ctx,
  enum AVPixelFormat in_pix_fmt, AVCodecContext *enc_ctx, int scale_algo)
{
  SwsOutputContext *sws_out_ctx;

  if (!(sws_out_ctx = malloc(sizeof(SwsOutputContext)))) {
    fprintf(stderr, "Failed to allocate SwsOutputContext.\n");
    return NULL;
  }

  sws_out_ctx->scaled_frame = NULL;

  if (!(sws_out_ctx->sws_ctx = sws_getContext(
    dec_ctx->width, dec_ctx->height, in_pix_fmt,
    enc_ctx->width, enc_ctx->height, enc_ctx->pix_fmt,
    scale_algo, NULL, NULL, NULL)))
  {
    fprintf(stderr, "Failed to get SwsContext.\n");
    return NULL;
  }

  if (!(sws_out_ctx->scaled_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate sws_out_ctx->scaled_frame.\n");
    return NULL;
  }

  sws_out_ctx->scaled_frame->width = enc_ctx->width;
  sws_out_ctx->scaled_frame->height = enc_ctx->height;
  sws_out_ctx->scaled_frame->format = enc_ctx->pix_fmt;

  if (av_frame_get_buffer(sws_out_ctx->scaled_frame, 0) < 0) {
    fprintf(stderr, "Failed to allocate buffers for frame.\n");
    return NULL;
  }

  return sws_out_ctx;
}

int sws_output_context_scale(SwsOutputContext *sws_out_ctx, AVFrame *frame)
{
  int ret = 0;

  if ((ret = av_frame_make_writable(sws_out_ctx->scaled_frame)) < 0) {
    fprintf(stderr, "Failed to make frame writable.\n");
    return ret;
  }

  ret = sws_scale(sws_out_ctx->sws_ctx, (const uint8_t * const *) frame->data,
    frame->linesize, 0, frame->height, sws_out_ctx->scaled_frame->data,
    sws_out_ctx->scaled_frame->linesize);

  sws_out_ctx->scaled_frame->pts = frame->pts;
  sws_out_ctx->scaled_frame->pkt_dts = frame->pkt_dts;

  return ret;
}

void sws_output_context_free(SwsOutputContext *sws_out_ctx)
{
  if (sws_out_ctx == NULL) return;
  sws_freeContext(sws_out_ctx->sws_ctx);
  av_frame_free(&sws_out_ctx->scaled_frame);
  free(sws_out_ctx);
}

typedef struct InputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *dec_ctx;
  enum AVHWDeviceType hwdev_type;
  AVPacket *init_pkt;
  AVFrame *hw_frame;
  AVFrame *sw_frame;
  int stream_idx;
} InputContext;

static enum AVPixelFormat hw_pix_fmt;

enum AVPixelFormat get_hw_fmt(AVCodecContext *dec_ctx, const enum AVPixelFormat *pix_fmts)
{
  const enum AVPixelFormat *pix_fmt;

  for (pix_fmt = pix_fmts; *pix_fmt != -1; pix_fmt++) {
    if (*pix_fmt == hw_pix_fmt) { return *pix_fmt; }
  }

  fprintf(stderr, "Failed to get hardware pixel format.\n");
  return AV_PIX_FMT_NONE;
}

int hw_decoder_init(InputContext *in_ctx)
{
  int ret = 0;
  AVBufferRef *hwdev_ctx;

  if ((ret = av_hwdevice_ctx_create(&hwdev_ctx,
    in_ctx->hwdev_type, NULL, NULL, 0)) < 0)
  {
    fprintf(stderr, "Failed to create hardware device context.\n"
      "Libav Error: %s.\n", av_err2str(ret));
    return ret;
  }

  in_ctx->dec_ctx->hw_device_ctx = av_buffer_ref(hwdev_ctx);
  return 0;
}

InputContext *open_input(const char *in_filename, unsigned int stream_idx, char *hwdev_name)
{
  int ret = 0;
  InputContext *in_ctx = NULL;
  AVStream *in_stream;
  const AVCodec *dec;
  const AVCodecHWConfig *config;

  if (!(in_ctx = malloc(sizeof(InputContext)))) {
    fprintf(stderr, "Failed to allocate InputContext.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  in_ctx->fmt_ctx = NULL;
  in_ctx->dec_ctx = NULL;
  in_ctx->hwdev_type = AV_HWDEVICE_TYPE_NONE;
  in_ctx->init_pkt = NULL;
  in_ctx->hw_frame = NULL;
  in_ctx->sw_frame = NULL;
  in_ctx->stream_idx = stream_idx;

  if ((ret =
    avformat_open_input(&in_ctx->fmt_ctx, in_filename, NULL, NULL)) < 0)
  {
    fprintf(stderr, "Failed to open AVFormatContext.\n");
    return NULL;
  }

  if ((ret = avformat_find_stream_info(in_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to find stream info.\n");
    return NULL;
  }

  if (stream_idx >= in_ctx->fmt_ctx->nb_streams) {
    fprintf(stderr, "Invalid stream index.\n");
    ret = -1;
    return NULL;
  }

  in_stream = in_ctx->fmt_ctx->streams[stream_idx];

  in_ctx->hwdev_type = av_hwdevice_find_type_by_name(hwdev_name);
  if (in_ctx->hwdev_type == AV_HWDEVICE_TYPE_NONE)
  {
    fprintf(stderr, "Device type %s is not supported.\n", hwdev_name);
    fprintf(stderr, "Available device types:\n");

    while((in_ctx->hwdev_type =
      av_hwdevice_iterate_types(in_ctx->hwdev_type)) != AV_HWDEVICE_TYPE_NONE)
    {
      fprintf(stderr, " %s\n", av_hwdevice_get_type_name(in_ctx->hwdev_type));
    }

    return NULL;
  }

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder for stream: '%d'.\n", stream_idx);
    ret = AVERROR(EINVAL);
    return NULL;
  }

  for (int i = 0;; i++)
  {
    if(!(config = avcodec_get_hw_config(dec, i))) {
      fprintf(stderr, "Decoder %s does not support device type %s.\n",
        dec->name, av_hwdevice_get_type_name(in_ctx->hwdev_type));
      return NULL;
    }

    if (
      config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
      config->device_type == in_ctx->hwdev_type
    ) {
      hw_pix_fmt = config->pix_fmt;
      break;
    }
  }

  if (!(in_ctx->dec_ctx = avcodec_alloc_context3(dec))) {
    fprintf(stderr,
      "Failed to allocate decoder for stream: '%d'.\n", stream_idx);
    ret = AVERROR(EINVAL);
    return NULL;
  }

  if ((ret = avcodec_parameters_to_context(in_ctx->dec_ctx,
    in_stream->codecpar)) < 0)
  {
    fprintf(stderr,
      "Failed to copy parameters from input stream: '%d' to decoder.\n",
      stream_idx);
    return NULL;
  }

  in_ctx->dec_ctx->get_format = get_hw_fmt;

  if (hw_decoder_init(in_ctx) < 0) {
    fprintf(stderr, "Failed to initialze hardware decoder.\n");
    return NULL;
  }

  if ((ret = avcodec_open2(in_ctx->dec_ctx, dec, NULL)) < 0) {
    fprintf(stderr, "Failed to open decoder for stream: '%d'.\n",
      stream_idx);
    return NULL;
  }

  if (!(in_ctx->hw_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  if (!(in_ctx->sw_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  if (!(in_ctx->init_pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  return in_ctx;
}

void close_input(InputContext *in_ctx)
{
  if (!in_ctx) return;
  avformat_close_input(&in_ctx->fmt_ctx);
  avcodec_free_context(&in_ctx->dec_ctx);
  av_frame_free(&in_ctx->hw_frame);
  av_frame_free(&in_ctx->sw_frame);
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
  const char *out_filename)
{
  int ret = 0;
  OutputContext *out_ctx = NULL;
  AVStream *in_stream, *out_stream;
  const AVCodec *enc;

  if (!(out_ctx = malloc(sizeof(OutputContext)))) {
    fprintf(stderr, "Failed to allocate OutputContext.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  out_ctx->fmt_ctx = NULL;
  out_ctx->enc_ctx = NULL;
  out_ctx->enc_pkt = NULL;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder.\n");
    ret = AVERROR(EINVAL);
    return NULL;
  }

  if (!(out_ctx->enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder.\n");
    ret = AVERROR(EINVAL);
    return NULL;
  }

  in_stream = in_ctx->fmt_ctx->streams[in_ctx->stream_idx];

  out_ctx->enc_ctx->time_base = in_stream->time_base;
  out_ctx->enc_ctx->framerate = in_stream->avg_frame_rate;

  out_ctx->enc_ctx->width = in_stream->codecpar->width;
  out_ctx->enc_ctx->height = in_stream->codecpar->height;
  out_ctx->enc_ctx->pix_fmt = in_stream->codecpar->format;

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
      fprintf(stderr, "Failed to set '%s'.\n", enc_params_opt);
      return NULL;
    }
  }

  if ((ret = avcodec_open2(out_ctx->enc_ctx, enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    return NULL;
  }

  if ((ret =
    avformat_alloc_output_context2(&out_ctx->fmt_ctx, NULL, NULL, out_filename)))
  {
    fprintf(stderr,
      "Failed to allocate output format context.\n");
    return NULL;
  }

  if ((ret = av_dict_copy(&out_ctx->fmt_ctx->metadata, in_ctx->fmt_ctx->metadata,
    AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr,
      "Failed to copy input metadata to output.\n");
    return NULL;
  }

  if (!(out_stream = avformat_new_stream(out_ctx->fmt_ctx, NULL))) {
    fprintf(stderr,
      "Failed to allocate new output stream.\n");
    return NULL;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    in_stream->metadata, AV_DICT_DONT_OVERWRITE)))
  {
    fprintf(stderr,
      "Failed to copy metadata from input stream to output stream.\n");
    return NULL;
  }

  if ((ret =
    avcodec_parameters_from_context(out_stream->codecpar, out_ctx->enc_ctx)))
  {
    fprintf(stderr,
      "Failed to copy parameters from encoder to output stream.\n");
    return NULL;
  }

  out_stream->time_base = out_ctx->enc_ctx->time_base;
  out_stream->r_frame_rate = in_stream->r_frame_rate;
  out_stream->avg_frame_rate = in_stream->avg_frame_rate;

  if (!(out_ctx->enc_pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  if (!(out_ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if ((ret =
      avio_open(&out_ctx->fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0)
    {
      fprintf(stderr, "Failed to create output file.\n");
      return NULL;
    }
  }

  if ((ret = avformat_write_header(out_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to write header to output.\n");
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

int encode_frame(AVStream *in_stream, SwsOutputContext *sws_out_ctx,
  OutputContext *out_ctx, AVStream *out_stream)
{
  int ret = 0;

  if ((ret = avcodec_send_frame(out_ctx->enc_ctx,
    sws_out_ctx->scaled_frame)) < 0)
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

int decode_packet(InputContext *in_ctx, AVStream *in_stream,
  SwsOutputContext **sws_out_ctx, OutputContext *out_ctx, AVStream *out_stream)
{
  int ret = 0;


  if ((ret = avcodec_send_packet(in_ctx->dec_ctx, in_ctx->init_pkt)) < 0) {
    fprintf(stderr, "Failed to send packet to decoder.\n");
    return ret;
  }

  while ((ret = avcodec_receive_frame(in_ctx->dec_ctx, in_ctx->hw_frame)) >= 0)
  {
    if ((ret = av_hwframe_transfer_data(in_ctx->sw_frame,
      in_ctx->hw_frame, 0)) < 0)
    {
      fprintf(stderr, "Failed to transfer hardware frame to software frame.\n"
        "Libav Error: %s.\n", av_err2str(ret));
    }
    if (in_ctx->hw_frame->pts < 0) {
      in_ctx->sw_frame->pts = in_ctx->hw_frame->pkt_dts;
    } else {
      in_ctx->sw_frame->pts = in_ctx->hw_frame->pts;
    }
    in_ctx->sw_frame->pkt_dts = in_ctx->hw_frame->pkt_dts;

    if (!*sws_out_ctx) {
      if (!(*sws_out_ctx = sws_output_context_alloc(in_ctx->dec_ctx,
        in_ctx->sw_frame->format, out_ctx->enc_ctx, OUTPUT_SCALE_ALGO)))
      {
        fprintf(stderr, "Failed to allocate SwsOutputContext.\n");
        ret = ENOMEM;
        return ret;
      }
    }

    if ((ret = sws_output_context_scale(*sws_out_ctx, in_ctx->sw_frame)) < 0) {
      fprintf(stderr, "Failed to scale frame.\n");
      return ret;
    }

    if ((ret = encode_frame(in_stream, *sws_out_ctx, out_ctx, out_stream)) < 0) {
      fprintf(stderr, "Failed to encode frame.\n");
      return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to receive frame from decoder.\n");
    return ret;
  }

  return 0;
}

int transcode(InputContext *in_ctx, AVStream *in_stream,
  SwsOutputContext **sws_out_ctx, OutputContext *out_ctx, AVStream *out_stream)
{
  int ret = 0;

  while ((ret = av_read_frame(in_ctx->fmt_ctx, in_ctx->init_pkt)) >= 0)
  {
    if (!(in_ctx->init_pkt->stream_index == in_ctx->stream_idx)) {
      av_packet_unref(in_ctx->init_pkt);
      continue;
    }

    if ((ret = decode_packet(in_ctx, in_stream, sws_out_ctx, out_ctx, out_stream)) < 0) {
      fprintf(stderr, "Failed to decode packet.\n");
      return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to read frame from input file.\n");
    return ret;
  }

  return 0;
}

int main(int argc, char **argv)
{
  int ret = 0;
  char *in_filename, *out_filename, *hw_dev_name, *codec,
    *enc_params = NULL, *enc_params_opt = NULL;
  InputContext *in_ctx = NULL;
  OutputContext *out_ctx = NULL;
  SwsOutputContext *sws_out_ctx = NULL;
  AVStream *in_stream, *out_stream;

  if (argc != 5 && argc != 6) {
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
  hw_dev_name = argv[3];
  codec = argv[4];

  if (argc == 6) {
    enc_params = argv[5];

    if ((ret = initialize_encoder_params(codec, &enc_params_opt)) < 0) {
      fprintf(stderr, "Failed to initialize encoder option params.\n");
      return -1;
    }
  }

  if (!(in_ctx = open_input(in_filename, 0, hw_dev_name))) {
    fprintf(stderr, "Failed to open input file: '%s'.\n", in_filename);
    goto end;
  }

  in_stream = in_ctx->fmt_ctx->streams[in_ctx->stream_idx];

  if (!(out_ctx =
    open_output(in_ctx, codec, enc_params, enc_params_opt, out_filename)))
  {
    fprintf(stderr, "Failed to open output file: '%s'.\n", out_filename);
    goto end;
  }

  out_stream = out_ctx->fmt_ctx->streams[0];

  if ((ret = transcode(in_ctx, in_stream, &sws_out_ctx, out_ctx, out_stream)) < 0) {
    fprintf(stderr, "Failed to transcode.\n");
    goto end;
  }

  in_ctx->init_pkt = NULL;
  if ((ret = decode_packet(in_ctx, in_stream, &sws_out_ctx, out_ctx, out_stream)) < 0) {
    fprintf(stderr, "Failed to flush decoder.\n");
    goto end;
  }

  sws_out_ctx->scaled_frame = NULL;
  if ((ret = encode_frame(in_stream, sws_out_ctx, out_ctx, out_stream)) < 0) {
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

  if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
    fprintf(stderr, "\nLibav Error: %s.\n", av_err2str(ret));
    return ret;
  }

  return 0;
}
