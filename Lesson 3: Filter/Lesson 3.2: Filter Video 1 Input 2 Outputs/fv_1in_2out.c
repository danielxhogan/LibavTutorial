#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

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
    fprintf(stderr, "Failed to allocate InputContext.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  in_ctx->fmt_ctx = NULL;
  in_ctx->dec_ctx = NULL;
  in_ctx->dec_frame = NULL;
  in_ctx->init_pkt = NULL;
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

  if (stream_idx >= in_ctx->fmt_ctx->nb_streams || stream_idx < 0) {
    fprintf(stderr, "Invalid stream index.\n");
    ret = -1;
    return NULL;
  }

  in_stream = in_ctx->fmt_ctx->streams[stream_idx];

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder for stream: '%d'.\n", stream_idx);
    ret = AVERROR(EINVAL);
    return NULL;
  }

  if (!(in_ctx->dec_ctx = avcodec_alloc_context3(dec))) {
    fprintf(stderr,
      "Failed to allocate decoder for stream: '%d'.\n", stream_idx);
    ret = AVERROR(EINVAL);
    return NULL;
  }

  if ((ret =
    avcodec_parameters_to_context(in_ctx->dec_ctx, in_stream->codecpar)) < 0)
  {
    fprintf(stderr,
      "Failed to copy parameters from input stream: '%d' to decoder.\n",
      stream_idx);
    return NULL;
  }

  if ((ret = avcodec_open2(in_ctx->dec_ctx, dec, NULL)) < 0) {
    fprintf(stderr, "Failed to open decoder for stream: '%d'.\n",
      stream_idx);
    return NULL;
  }

  if (!(in_ctx->dec_frame = av_frame_alloc())) {
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
  AVCodecContext *enc_ctx1;
  AVCodecContext *enc_ctx2;
  AVPacket *enc_pkt;
} OutputContext;

int enc_ctx_init(AVCodecContext **enc_ctx,
  AVStream *in_stream, int width, int height,
  const char *codec, char *enc_params, char *enc_params_opt, int tonemap)
{
  int ret = 0;
  const AVCodec *enc;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder.\n");
    ret = AVERROR(EINVAL);
    return ret;
  }

  if (!(*enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder.\n");
    ret = AVERROR(EINVAL);
    return ret;
  }


  (*enc_ctx)->time_base = in_stream->time_base;
  (*enc_ctx)->framerate = in_stream->avg_frame_rate;

  (*enc_ctx)->width = width;
  (*enc_ctx)->height = height;

  if (tonemap) {
    (*enc_ctx)->profile = AV_PROFILE_HEVC_MAIN;
    (*enc_ctx)->pix_fmt = AV_PIX_FMT_YUV420P;
    (*enc_ctx)->color_range = AVCOL_RANGE_MPEG;
    (*enc_ctx)->color_primaries = AVCOL_PRI_BT709;
    (*enc_ctx)->color_trc = AVCOL_TRC_BT709;
    (*enc_ctx)->colorspace = AVCOL_SPC_BT709;
    (*enc_ctx)->chroma_sample_location = AVCHROMA_LOC_LEFT;
  }
  else {
    (*enc_ctx)->pix_fmt = in_stream->codecpar->format;
    (*enc_ctx)->color_range = in_stream->codecpar->color_range;
    (*enc_ctx)->color_primaries = in_stream->codecpar->color_primaries;
    (*enc_ctx)->color_trc = in_stream->codecpar->color_trc;
    (*enc_ctx)->colorspace = in_stream->codecpar->color_space;
    (*enc_ctx)->chroma_sample_location = in_stream->codecpar->chroma_location;
  }

  (*enc_ctx)->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (enc_params && enc_params_opt) {
    if ((ret = av_opt_set((*enc_ctx)->priv_data,
      enc_params_opt, enc_params, 0)) < 0)
    {
      fprintf(stderr, "Failed to set %s.\n", enc_params_opt);
      return ret;
    }
  }

  if ((ret = avcodec_open2((*enc_ctx), enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    return ret;
  }

  return 0;
}

int output_stream_init(OutputContext *out_ctx,
  AVCodecContext *enc_ctx, AVStream *in_stream)
{
  int ret = 0;
  AVStream *out_stream;

  if (!(out_stream = avformat_new_stream(out_ctx->fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate new output stream.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    in_stream->metadata, AV_DICT_DONT_OVERWRITE)))
  {
    fprintf(stderr,
      "Failed to copy metadata from input stream to output stream.\n");
    return ret;
  }

  if ((ret =
    avcodec_parameters_from_context(out_stream->codecpar, enc_ctx)))
  {
    fprintf(stderr,
      "Failed to copy parameters from encoder to output stream.\n");
    return ret;
  }

  out_stream->time_base = enc_ctx->time_base;
  out_stream->r_frame_rate = in_stream->r_frame_rate;
  out_stream->avg_frame_rate = in_stream->avg_frame_rate;

  return 0;
}

OutputContext *open_output(InputContext *in_ctx, int width, int height,
  const char *codec, char *enc_params, char *enc_params_opt,
  const char *out_filename)
{
  int ret = 0;
  OutputContext *out_ctx = NULL;
  AVStream *in_stream;

  if (!(out_ctx = malloc(sizeof(OutputContext)))) {
    fprintf(stderr, "Failed to allocate OutputContext.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  out_ctx->fmt_ctx = NULL;
  out_ctx->enc_ctx1 = NULL;
  out_ctx->enc_ctx2 = NULL;
  out_ctx->enc_pkt = NULL;

  in_stream = in_ctx->fmt_ctx->streams[in_ctx->stream_idx];

  if ((ret = enc_ctx_init(&out_ctx->enc_ctx1, in_stream,
    in_stream->codecpar->width, in_stream->codecpar->height,
    codec, enc_params, enc_params_opt, 0)) < 0)
  {
    fprintf(stderr,
      "Failed to initialize encoder context for first encoder.\n");
    return NULL;
  }

  if ((ret = enc_ctx_init(&out_ctx->enc_ctx2, in_stream,
    width, height,
    codec, enc_params, enc_params_opt, 1)) < 0)
  {
    fprintf(stderr,
      "Failed to initialize encoder context for second encoder.\n");
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

  if ((ret = output_stream_init(out_ctx, out_ctx->enc_ctx1, in_stream)) < 0) {
    fprintf(stderr, "Failed to initialize output stream for first stream.\n");
    return NULL;
  }

  if ((ret = output_stream_init(out_ctx, out_ctx->enc_ctx2, in_stream)) < 0) {
    fprintf(stderr, "Failed to initialize output stream for first stream.\n");
    return NULL;
  }

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
  avcodec_free_context(&out_ctx->enc_ctx1);
  avcodec_free_context(&out_ctx->enc_ctx2);
  free(out_ctx);
}

typedef struct FilterContext {
  AVFilterContext *buffersrc_ctx;
  AVFilterContext *buffersink_ctx1;
  AVFilterContext *buffersink_ctx2;
  AVFilterGraph *filter_graph;
  AVFrame *filtered_frame1;
  AVFrame *filtered_frame2;
} FilterContext;

int buffersink_ctx_init(AVFilterContext **buffersink_ctx,
  AVFilterGraph *filter_graph, AVStream *in_stream, const char *pix_fmt)
{
  int ret = 0;
  // const char *pix_fmt;
  const AVFilter *buffersink = avfilter_get_by_name("buffersink");

  if (!(*buffersink_ctx =
    avfilter_graph_alloc_filter(filter_graph, buffersink, "out")))
  {
    fprintf(stderr, "Failed to create buffer sink.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  // pix_fmt = av_get_pix_fmt_name(in_stream->codecpar->format);

  if ((ret = av_opt_set(*buffersink_ctx, "pixel_formats",
    pix_fmt, AV_OPT_SEARCH_CHILDREN)))
  {
    fprintf(stderr, "Failed to set pixel format on buffersink.\n");
    return ret;
  }

  if ((ret = avfilter_init_dict(*buffersink_ctx, NULL))) {
    fprintf(stderr, "Failed to initialize buffersink.\n");
    return ret;
  }

  return 0;
}

FilterContext *filter_context_init(InputContext *in_ctx, char *filter_descr)
{
  int ret = 0;
  char args[512];

  const AVFilter *buffersrc = avfilter_get_by_name("buffer");
  AVStream *in_stream = in_ctx->fmt_ctx->streams[in_ctx->stream_idx];

  FilterContext *flt_ctx = NULL;
  AVFilterInOut *outputs = NULL;
  AVFilterInOut *inputs = NULL;

  if (!(flt_ctx = malloc(sizeof(FilterContext)))) {
    fprintf(stderr, "Failed to allocate FilterContext.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  flt_ctx->buffersink_ctx1 = NULL;
  flt_ctx->buffersink_ctx2 = NULL;
  flt_ctx->buffersrc_ctx = NULL;
  flt_ctx->filter_graph = NULL;
  flt_ctx->filtered_frame1 = NULL;
  flt_ctx->filtered_frame2 = NULL;

  if (!(outputs = avfilter_inout_alloc())) {
    fprintf(stderr, "Failed to allocate outputs.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(inputs = avfilter_inout_alloc())) {
    fprintf(stderr, "Failed to allocate outputs.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(flt_ctx->filter_graph = avfilter_graph_alloc())) {
    fprintf(stderr, "Failed to allocate filter graph.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  snprintf(args, sizeof(args),
    "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
    in_ctx->dec_ctx->width, in_ctx->dec_ctx->height, in_ctx->dec_ctx->pix_fmt,
    in_stream->time_base.num, in_stream->time_base.den,
    in_ctx->dec_ctx->sample_aspect_ratio.num,
    in_ctx->dec_ctx->sample_aspect_ratio.den);

  if ((ret = avfilter_graph_create_filter(&flt_ctx->buffersrc_ctx, buffersrc,
    "in", args, NULL, flt_ctx->filter_graph)) < 0)
  {
    fprintf(stderr, "Failed to create buffer source.\n");
    goto end;
  }

  if ((ret = buffersink_ctx_init(&flt_ctx->buffersink_ctx1,
    flt_ctx->filter_graph, in_stream, "yuv420p10le")) < 0)
  {
    fprintf(stderr, "Failed to initialize first buffer sink context.\n");
    goto end;
  }

  if ((ret = buffersink_ctx_init(&flt_ctx->buffersink_ctx2,
    flt_ctx->filter_graph, in_stream, "yuv420p")) < 0)
  {
    fprintf(stderr, "Failed to initialize first buffer sink context.\n");
    goto end;
  }

  outputs->name = av_strdup("in");
  outputs->filter_ctx = flt_ctx->buffersrc_ctx;
  outputs->pad_idx = 0;
  outputs->next = NULL;

  inputs->name = av_strdup("out1");
  inputs->filter_ctx = flt_ctx->buffersink_ctx1;
  inputs->pad_idx = 0;
  inputs->next = avfilter_inout_alloc();

  inputs->next->name = av_strdup("out2");
  inputs->next->filter_ctx = flt_ctx->buffersink_ctx2;
  inputs->next->pad_idx = 0;
  inputs->next->next = NULL;

  if ((ret = avfilter_graph_parse_ptr(flt_ctx->filter_graph,
    filter_descr, &inputs, &outputs, NULL)) < 0)
  {
    fprintf(stderr, "Failed to configure filter graph.\n");
    goto end;
  }

  if ((ret = avfilter_graph_config(flt_ctx->filter_graph, NULL)) < 0) {
    fprintf(stderr, "Failed to configure filter graph.\n");
    goto end;
  }

  if (!(flt_ctx->filtered_frame1 = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(flt_ctx->filtered_frame2 = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

end:
  avfilter_inout_free(&inputs);
  avfilter_inout_free(&outputs);

  if (ret < 0) { return NULL; }
  return flt_ctx;
}

void filter_context_free(FilterContext *flt_ctx)
{
  if (!flt_ctx) return;
  avfilter_graph_free(&flt_ctx->filter_graph);
  av_frame_free(&flt_ctx->filtered_frame1);
  av_frame_free(&flt_ctx->filtered_frame2);
  free(flt_ctx);
}

#define OUTPUT_PIX_FMT AV_PIX_FMT_YUV420P
#define OUTPUT_SCALE_ALGO SWS_BICUBIC

typedef struct SwsOutputContext {
  struct SwsContext *sws_ctx;
  int width;
  int height;
  enum AVPixelFormat pix_fmt;
  int scale_algo;
  AVFrame *scaled_frame;
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
  sws_out_ctx->scaled_frame = NULL;

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

  if (!(sws_out_ctx->scaled_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate sws_out_ctx->scaled_frame.\n");
    return NULL;
  }

  sws_out_ctx->scaled_frame->width = width;
  sws_out_ctx->scaled_frame->height = height;
  sws_out_ctx->scaled_frame->format = pix_fmt;

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

int encode_frame(AVStream *in_stream, OutputContext *out_ctx,
  AVCodecContext *enc_ctx, AVFrame *frame, int out_stream_idx)
{
  int ret = 0;
  AVStream *out_stream = out_ctx->fmt_ctx->streams[out_stream_idx];

  if ((ret = avcodec_send_frame(enc_ctx, frame)) < 0) {
    fprintf(stderr, "Failed to send frame to encoder.\n");
    return ret;
  }

  while ((ret =
    avcodec_receive_packet(enc_ctx, out_ctx->enc_pkt)) >= 0)
  {
    out_ctx->enc_pkt->stream_index = out_stream_idx;

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

  return 0;
}

int filter_frame(InputContext *in_ctx, AVStream *in_stream,
  OutputContext *out_ctx, FilterContext *flt_ctx, SwsOutputContext *sws_out_ctx)
{
  int ret = 0;

  if ((ret = av_buffersrc_add_frame_flags(flt_ctx->buffersrc_ctx,
    in_ctx->dec_frame, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0)
  {
    fprintf(stderr, "Failed to add frame to buffer source.\n");
    return ret;
  }

  while ((ret = av_buffersink_get_frame(flt_ctx->buffersink_ctx1,
    flt_ctx->filtered_frame1)) >= 0)
  {
    if ((ret = encode_frame(in_stream, out_ctx, out_ctx->enc_ctx1,
      flt_ctx->filtered_frame1, 0)) < 0)
    {
    fprintf(stderr, "Failed to encode frame.\n");
    return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to get frame from first buffer sink.\n");
    return ret;
  }

  while ((ret = av_buffersink_get_frame(flt_ctx->buffersink_ctx2,
    flt_ctx->filtered_frame2)) >= 0)
  {
    if ((ret =
      sws_output_context_scale(sws_out_ctx, flt_ctx->filtered_frame2)) < 0)
    {
      fprintf(stderr, "Failed to scale frame.\n");
      return ret;
    }

    if ((ret = encode_frame(in_stream, out_ctx, out_ctx->enc_ctx2,
      sws_out_ctx->scaled_frame, 1)) < 0)
    {
    fprintf(stderr, "Failed to encode frame.\n");
    return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to get frame from second buffer sink.\n");
    return ret;
  }

  return 0;
}

int decode_packet(InputContext *in_ctx, AVStream *in_stream,
  OutputContext *out_ctx, FilterContext *flt_ctx, SwsOutputContext *sws_out_ctx)
{
  int ret = 0;

  if ((ret = avcodec_send_packet(in_ctx->dec_ctx, in_ctx->init_pkt)) < 0) {
    fprintf(stderr, "Failed to send packet to decoder.\n");
    return ret;
  }

  while ((ret = avcodec_receive_frame(in_ctx->dec_ctx, in_ctx->dec_frame)) >= 0)
  {
    in_ctx->dec_frame->pict_type = AV_PICTURE_TYPE_NONE;

    if ((ret = filter_frame(in_ctx, in_stream, out_ctx, flt_ctx, sws_out_ctx)) < 0) {
      fprintf(stderr, "Failed to filter frame.\n");
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
  OutputContext *out_ctx, FilterContext *flt_ctx, SwsOutputContext *sws_out_ctx)
{
  int ret = 0;

  while ((ret = av_read_frame(in_ctx->fmt_ctx, in_ctx->init_pkt)) >= 0)
  {
    if (!(in_ctx->init_pkt->stream_index == in_ctx->stream_idx)) {
      av_packet_unref(in_ctx->init_pkt);
      continue;
    }

    if ((ret = decode_packet(in_ctx, in_stream, out_ctx, flt_ctx, sws_out_ctx)) < 0) {
      fprintf(stderr, "Failed to decode and filter packet.\n");
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
  int width, height, ret = 0;
  char *in_filename, *out_filename, *filter_descr, *width_str, *height_str,
    *codec, *enc_params = NULL, *enc_params_opt = NULL;
  FilterContext *flt_ctx = NULL;
  SwsOutputContext *sws_out_ctx = NULL;
  InputContext *in_ctx = NULL;
  OutputContext *out_ctx = NULL;
  AVStream *in_stream;

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
  filter_descr = argv[3];
  width_str = argv[4];
  height_str = argv[5];
  codec = argv[6];

  if (!(width = atoi(width_str))) {
    fprintf(stderr, "Invalid value entered for width.\n");
    goto end;
  }

  if (!(height = atoi(height_str))) {
    fprintf(stderr, "Invalid value entered for height.\n");
    goto end;
  }

  if (argc == 8) {
    enc_params = argv[7];

    if ((ret = initialize_encoder_params(codec, &enc_params_opt)) < 0) {
      fprintf(stderr, "Failed to initialize encoder option params.\n");
      return -1;
    }
  }

  if (!(in_ctx = open_input(in_filename, 0))) {
    fprintf(stderr, "Failed to open input file: '%s'.\n", in_filename);
    goto end;
  }

  in_stream = in_ctx->fmt_ctx->streams[in_ctx->stream_idx];

  if (!(out_ctx = open_output(in_ctx, width, height,
    codec, enc_params, enc_params_opt, out_filename)))
  {
    fprintf(stderr, "Failed to open output file: '%s'.\n", out_filename);
    goto end;
  }

  if (!(flt_ctx = filter_context_init(in_ctx, filter_descr))) {
    fprintf(stderr, "Failed to initialize filter context.\n");
    goto end;
  }
  if (!(sws_out_ctx = sws_output_context_alloc(width, height,
    OUTPUT_PIX_FMT, OUTPUT_SCALE_ALGO, in_stream->codecpar)))
  {
    fprintf(stderr, "Failed to allocate SwsOutputContext.\n");
    ret = ENOMEM;
    goto end;
  }

  if ((ret = transcode(in_ctx, in_stream, out_ctx, flt_ctx, sws_out_ctx)) < 0)
  {
    fprintf(stderr, "Failed to transcode.\n");
    goto end;
  }

  in_ctx->init_pkt = NULL;
  if ((ret = decode_packet(in_ctx, in_stream,
    out_ctx, flt_ctx, sws_out_ctx)) < 0)
  {
    fprintf(stderr, "Failed to flush decoder.\n");
    goto end;
  }

  in_ctx->dec_frame = NULL;
  if ((ret = filter_frame(in_ctx, in_stream,
    out_ctx, flt_ctx, sws_out_ctx)) < 0)
  {
    fprintf(stderr, "Failed to flush filter.\n");
    goto end;
  }

  flt_ctx->filtered_frame1 = NULL;
  if ((ret = encode_frame(in_stream, out_ctx, out_ctx->enc_ctx1,
    flt_ctx->filtered_frame1, 0)) < 0)
  {
    fprintf(stderr, "Failed to flush encoder.\n");
    goto end;
  }

  flt_ctx->filtered_frame2 = NULL;
  if ((ret = encode_frame(in_stream, out_ctx, out_ctx->enc_ctx2,
    flt_ctx->filtered_frame2, 1)) < 0)
  {
    fprintf(stderr, "Failed to flush encoder.\n");
    goto end;
  }

  if ((ret = av_write_trailer(out_ctx->fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  filter_context_free(flt_ctx);
  sws_output_context_free(sws_out_ctx);
  close_input(in_ctx);
  close_output(out_ctx);

  if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return ret;
  }

  return 0;
}
