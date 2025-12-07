#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

typedef struct InputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *v_dec_ctx;
  AVCodecContext *s_dec_ctx;
  AVFrame *v_dec_frame;
  AVFrame *s_dec_frame;
  AVSubtitle *dec_sub;
  AVPacket *init_pkt;
  int v_stream_idx;
  int s_stream_idx;
} InputContext;

int initialize_decoder(AVCodecContext **dec_ctx, AVStream *in_stream)
{
  int ret = 0;
  const AVCodec *dec;

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder.\n");
    ret = AVERROR(EINVAL);
    return ret;
  }

  if (!(*dec_ctx = avcodec_alloc_context3(dec))) {
    fprintf(stderr,
      "Failed to allocate decoder.\n");
    ret = AVERROR(EINVAL);
    return ret;
  }

  if ((ret =
    avcodec_parameters_to_context(*dec_ctx, in_stream->codecpar)) < 0)
  {
    fprintf(stderr,
      "Failed to copy parameters from input stream to decoder.\n");
    return ret;
  }

  (*dec_ctx)->time_base = in_stream->time_base;

  printf("height: %d\n", in_stream->codecpar->height);
  printf("width: %d\n", in_stream->codecpar->width);
  printf("pix_fmt: %d\n", in_stream->codecpar->format);

  if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
    fprintf(stderr, "Failed to open decoder.\n");
    return ret;
  }

  return 0;
}

InputContext *open_input(const char *in_filename,
  unsigned int v_stream_idx, unsigned int s_stream_idx)
{
  int ret = 0;
  InputContext *in_ctx = NULL;
  AVDictionary *opts = NULL;
  AVStream *v_stream;
  AVStream *s_stream;

  if (!(in_ctx = malloc(sizeof(InputContext)))) {
    fprintf(stderr, "Failed to allocate InputContext.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  in_ctx->fmt_ctx = NULL;
  in_ctx->v_dec_ctx = NULL;
  in_ctx->s_dec_ctx = NULL;
  in_ctx->v_dec_frame = NULL;
  in_ctx->s_dec_frame = NULL;
  in_ctx->dec_sub = NULL;
  in_ctx->init_pkt = NULL;
  in_ctx->v_stream_idx = v_stream_idx;
  in_ctx->s_stream_idx = s_stream_idx;

  if ((ret = av_dict_set(&opts, "probesize", "50000000", 0)) < 0) {
    fprintf(stderr, "Failed to set probesize option.\n");
    return NULL;
}

if ((ret = av_dict_set(&opts, "analyzeduration", "10000000", 0)) < 0) {
    fprintf(stderr, "Failed to set analyzeduration option.\n");
    return NULL;
}

  if ((ret =
    avformat_open_input(&in_ctx->fmt_ctx, in_filename, NULL, &opts)) < 0)
  {
    fprintf(stderr, "Failed to open AVFormatContext.\n");
    return NULL;
  }

  if ((ret = avformat_find_stream_info(in_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to find stream info.\n");
    return NULL;
  }

  if (v_stream_idx >= in_ctx->fmt_ctx->nb_streams || v_stream_idx < 0) {
    fprintf(stderr, "Invalid stream index.\n");
    ret = -1;
    return NULL;
  }

  v_stream = in_ctx->fmt_ctx->streams[v_stream_idx];
  s_stream = in_ctx->fmt_ctx->streams[s_stream_idx];

  if ((ret = initialize_decoder(&in_ctx->v_dec_ctx, v_stream)) < 0) {
    fprintf(stderr, "Failed to initialze decoder for stream: '%d'\n",
      v_stream_idx);
    return NULL;
  }

  if ((ret = initialize_decoder(&in_ctx->s_dec_ctx, s_stream)) < 0) {
    fprintf(stderr, "Failed to initialze decoder for stream: '%d'\n",
      s_stream_idx);
    return NULL;
  }

  if (!(in_ctx->v_dec_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  if (!(in_ctx->s_dec_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    return NULL;
  }

  if (!(in_ctx->dec_sub = av_mallocz(sizeof(AVSubtitle)))) {
    fprintf(stderr, "Failed to allocate AVSubtitle.\n");
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
  avcodec_free_context(&in_ctx->v_dec_ctx);
  avcodec_free_context(&in_ctx->s_dec_ctx);
  av_frame_free(&in_ctx->v_dec_frame);
  av_frame_free(&in_ctx->s_dec_frame);
  avsubtitle_free(in_ctx->dec_sub);
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

  in_stream = in_ctx->fmt_ctx->streams[in_ctx->v_stream_idx];

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

  if ((ret =
    av_opt_set(out_ctx->enc_ctx->priv_data, "preset", "ultrafast", 0)) < 0)
  {
    fprintf(stderr, "Failed to set preset.\n");
    return NULL;
  }

  if (enc_params && enc_params_opt) {
    if ((ret = av_opt_set(out_ctx->enc_ctx->priv_data,
      enc_params_opt, enc_params, 0)) < 0)
    {
      fprintf(stderr, "Failed to set %s.\n", enc_params_opt);
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

typedef struct FilterContext {
  AVFilterContext *v_buffersrc_ctx;
  AVFilterContext *s_buffersrc_ctx;
  AVFilterContext *buffersink_ctx;
  AVFilterGraph *filter_graph;
  AVFrame *filtered_frame;
} FilterContext;

#define OUTPUT_SUB_PIX_FMT AV_PIX_FMT_RGBA

FilterContext *filter_context_init(InputContext *in_ctx, char *filter_descr)
{
  int ret = 0;
  char v_args[512];
  char s_args[512];
  const char *pix_fmt;

  const AVFilter *buffersrc = avfilter_get_by_name("buffer");
  const AVFilter *buffersink = avfilter_get_by_name("buffersink");

  FilterContext *flt_ctx = NULL;
  AVFilterInOut *outputs = NULL;
  AVFilterInOut *inputs = NULL;

  if (!(flt_ctx = malloc(sizeof(FilterContext)))) {
    fprintf(stderr, "Failed to allocate FilterContext.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  flt_ctx->v_buffersrc_ctx = NULL;
  flt_ctx->s_buffersrc_ctx = NULL;
  flt_ctx->buffersink_ctx = NULL;
  flt_ctx->filter_graph = NULL;
  flt_ctx->filtered_frame = NULL;

  if (!(outputs = avfilter_inout_alloc())) {
    fprintf(stderr, "Failed to allocate outputs.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(outputs->next = avfilter_inout_alloc())) {
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

  snprintf(v_args, sizeof(v_args),
    "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
    in_ctx->v_dec_ctx->width,
    in_ctx->v_dec_ctx->height,
    in_ctx->v_dec_ctx->pix_fmt,
    in_ctx->fmt_ctx->streams[in_ctx->v_stream_idx]->time_base.num,
    in_ctx->fmt_ctx->streams[in_ctx->v_stream_idx]->time_base.den,
    in_ctx->v_dec_ctx->sample_aspect_ratio.num,
    in_ctx->v_dec_ctx->sample_aspect_ratio.den);

  if ((ret = avfilter_graph_create_filter(&flt_ctx->v_buffersrc_ctx, buffersrc,
    "in1", v_args, NULL, flt_ctx->filter_graph)) < 0)
  {
    fprintf(stderr, "Failed to create buffer source.\n");
    goto end;
  }

  snprintf(s_args, sizeof(s_args),
    "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
    in_ctx->v_dec_ctx->width, in_ctx->v_dec_ctx->height, OUTPUT_SUB_PIX_FMT,
    in_ctx->fmt_ctx->streams[in_ctx->s_stream_idx]->time_base.num,
    in_ctx->fmt_ctx->streams[in_ctx->s_stream_idx]->time_base.den,
    in_ctx->s_dec_ctx->sample_aspect_ratio.num,
    in_ctx->s_dec_ctx->sample_aspect_ratio.den);

  if ((ret = avfilter_graph_create_filter(&flt_ctx->s_buffersrc_ctx, buffersrc,
    "in2", s_args, NULL, flt_ctx->filter_graph)) < 0)
  {
    fprintf(stderr, "Failed to create buffer source.\n");
    goto end;
  }

  if (!(flt_ctx->buffersink_ctx =
    avfilter_graph_alloc_filter(flt_ctx->filter_graph, buffersink, "out")))
  {
    fprintf(stderr, "Failed to create buffer sink.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  pix_fmt = av_get_pix_fmt_name(
    in_ctx->fmt_ctx->streams[in_ctx->v_stream_idx]->codecpar->format);

  if ((ret = av_opt_set(flt_ctx->buffersink_ctx, "pixel_formats",
    pix_fmt, AV_OPT_SEARCH_CHILDREN)))
  {
    fprintf(stderr, "Failed to set pixel format on buffersink.\n");
    goto end;
  }

  if ((ret = avfilter_init_dict(flt_ctx->buffersink_ctx, NULL))) {
    fprintf(stderr, "Failed to initialize buffersink.\n");
    goto end;
  }

  outputs->name = av_strdup("in1");
  outputs->filter_ctx = flt_ctx->v_buffersrc_ctx;
  outputs->pad_idx = 0;

  outputs->next->name = av_strdup("in2");
  outputs->next->filter_ctx = flt_ctx->s_buffersrc_ctx;
  outputs->next->pad_idx = 0;
  outputs->next->next = NULL;

  inputs->name = av_strdup("out");
  inputs->filter_ctx = flt_ctx->buffersink_ctx;
  inputs->pad_idx = 0;
  inputs->next = NULL;

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

  if (!(flt_ctx->filtered_frame = av_frame_alloc())) {
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
  av_frame_free(&flt_ctx->filtered_frame);
  free(flt_ctx);
}

#define OUTPUT_SCALE_ALGO SWS_BICUBIC

typedef struct SubToFrameContext {
  struct SwsContext *sws_ctx;
  enum AVPixelFormat in_pix_fmt;
  enum AVPixelFormat out_pix_fmt;
  long width_ratio;
  long height_ratio;
  int scale_algo;
  AVFrame *subtitle_frame;
} SubToFrameContext;

SubToFrameContext *sub_to_frame_context_alloc(InputContext *in_ctx)
{
  enum AVPixelFormat in_pix_fmt = AV_PIX_FMT_PAL8;
  enum AVPixelFormat out_pix_fmt = OUTPUT_SUB_PIX_FMT;

  SubToFrameContext *sub_to_frame_ctx;

  if (!(sub_to_frame_ctx = malloc(sizeof(SubToFrameContext)))) {
    fprintf(stderr, "Failed to allocate SwsOutputContext.\n");
    return NULL;
  }

  sub_to_frame_ctx->sws_ctx = NULL;
  sub_to_frame_ctx->in_pix_fmt = in_pix_fmt;
  sub_to_frame_ctx->out_pix_fmt = out_pix_fmt;
  sub_to_frame_ctx->scale_algo = OUTPUT_SCALE_ALGO;
  sub_to_frame_ctx->subtitle_frame = NULL;

  if (!(sub_to_frame_ctx->subtitle_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate sub_to_frame_ctx->subtitle_frame.\n");
    return NULL;
  }

  sub_to_frame_ctx->subtitle_frame->width = in_ctx->v_dec_ctx->width;
  sub_to_frame_ctx->subtitle_frame->height = in_ctx->v_dec_ctx->height;
  sub_to_frame_ctx->subtitle_frame->format = out_pix_fmt;

  if (av_frame_get_buffer(sub_to_frame_ctx->subtitle_frame, 0) < 0) {
    fprintf(stderr, "Failed to allocate buffers for frame.\n");
    return NULL;
  }

  sub_to_frame_ctx->width_ratio =
    in_ctx->v_dec_ctx->width /
    in_ctx->s_dec_ctx->width;

  sub_to_frame_ctx->height_ratio =
    in_ctx->v_dec_ctx->height /
    in_ctx->s_dec_ctx->height;

  return sub_to_frame_ctx;
}

int sub_to_frame_sws_context_alloc(SubToFrameContext *sub_to_frame_ctx,
  AVSubtitleRect *rect)
{
  int out_width = rect->w * sub_to_frame_ctx->width_ratio;
  int out_height = rect->h * sub_to_frame_ctx->height_ratio;

  if (!(sub_to_frame_ctx->sws_ctx = sws_getCachedContext(
    sub_to_frame_ctx->sws_ctx,
    rect->w, rect->h, sub_to_frame_ctx->in_pix_fmt,
    out_width, out_height, sub_to_frame_ctx->out_pix_fmt,
    sub_to_frame_ctx->scale_algo, NULL, NULL, NULL)))
  {
    fprintf(stderr, "Failed to get SwsContext.\n");
    return AVERROR_UNKNOWN;
  }

  return 0;
}

int sub_to_frame_convert(SubToFrameContext *sub_to_frame_ctx,
  InputContext *in_ctx)
{
  int ret = 0;
  AVSubtitle *sub = in_ctx->dec_sub;
  uint8_t *dst_data[AV_NUM_DATA_POINTERS] = { NULL };

  memset(sub_to_frame_ctx->subtitle_frame->data[0], 0,
    sub_to_frame_ctx->subtitle_frame->height *
    sub_to_frame_ctx->subtitle_frame->linesize[0]);

  for (int i = 0; i < sub->num_rects; i++)
  {
    dst_data[0] = sub_to_frame_ctx->subtitle_frame->data[0] +
      (sub->rects[i]->y *
        sub_to_frame_ctx->subtitle_frame->linesize[0] *
        sub_to_frame_ctx->height_ratio) +
      (sub->rects[i]->x * 4 * sub_to_frame_ctx->width_ratio);

    if ((ret =
      sub_to_frame_sws_context_alloc(sub_to_frame_ctx, sub->rects[i])) < 0)
    {
      fprintf(stderr,
        "Failed to initialize sws_context for sub_to_frame_ctx.\n");
      return ret;
    }

    if ((ret = av_frame_make_writable(sub_to_frame_ctx->subtitle_frame)) < 0) {
      fprintf(stderr, "Failed to make frame writable.\n");
      return ret;
    }

    ret = sws_scale(sub_to_frame_ctx->sws_ctx,
      (const uint8_t * const *) sub->rects[i]->data,
      sub->rects[i]->linesize,
      0, sub->rects[i]->h,
      dst_data,
      sub_to_frame_ctx->subtitle_frame->linesize
    );
  }

  sub_to_frame_ctx->subtitle_frame->pts = sub->pts;
  sub_to_frame_ctx->subtitle_frame->pkt_dts = in_ctx->init_pkt->dts;

  return 0;
}

void sub_to_frame_context_free(SubToFrameContext *sub_to_frame_ctx)
{
  if (sub_to_frame_ctx == NULL) return;
  sws_freeContext(sub_to_frame_ctx->sws_ctx);
  av_frame_free(&sub_to_frame_ctx->subtitle_frame);
  free(sub_to_frame_ctx);
}


int encode_frame(InputContext *in_ctx, OutputContext *out_ctx,
  FilterContext *flt_ctx)
{
  int ret = 0;

  if ((ret =
    avcodec_send_frame(out_ctx->enc_ctx, flt_ctx->filtered_frame)) < 0)
  {
    fprintf(stderr, "Failed to send frame to encoder.\n");
    return ret;
  }

  while ((ret =
    avcodec_receive_packet(out_ctx->enc_ctx, out_ctx->enc_pkt)) >= 0)
  {
    out_ctx->enc_pkt->stream_index = 0;

    av_packet_rescale_ts(out_ctx->enc_pkt,
      in_ctx->v_dec_ctx->time_base, out_ctx->enc_ctx->time_base);

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

int filter_encode_frame(InputContext *in_ctx, OutputContext *out_ctx,
  FilterContext *flt_ctx, AVFilterContext *buffersrc_ctx, AVFrame *frame)
{
  int ret = 0;

  if ((ret = av_buffersrc_add_frame_flags(buffersrc_ctx,
    frame, AV_BUFFERSRC_FLAG_KEEP_REF)) < 0)
  {
    fprintf(stderr, "Failed to add frame to buffer source.\n");
    return ret;
  }

  while ((ret = av_buffersink_get_frame(flt_ctx->buffersink_ctx,
    flt_ctx->filtered_frame)) >= 0)
  {
    if ((ret = encode_frame(in_ctx, out_ctx, flt_ctx)) < 0) {
    fprintf(stderr, "Failed to encode frame.\n");
    return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to get frame from buffer sink.\n");
    return ret;
  }

  return 0;
}

int decode_video_packet(InputContext *in_ctx, OutputContext *out_ctx,
  FilterContext *flt_ctx, SubToFrameContext *sub_to_frame_ctx)
{
  int ret = 0;

  if ((ret = avcodec_send_packet(in_ctx->v_dec_ctx, in_ctx->init_pkt)) < 0) {
    fprintf(stderr, "Failed to send packet to decoder.\n");
    return ret;
  }

  while ((ret =
    avcodec_receive_frame(in_ctx->v_dec_ctx, in_ctx->v_dec_frame)) >= 0)
  {
    if ((ret = filter_encode_frame(in_ctx, out_ctx,
      flt_ctx, flt_ctx->v_buffersrc_ctx, in_ctx->v_dec_frame)) < 0)
    {
      fprintf(stderr, "Failed to filter and encode frame.");
      return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to receive frame from decoder.\n");
    return ret;
  }

  return 0;
}

int decode_subtitle_packet(InputContext *in_ctx, OutputContext *out_ctx,
  FilterContext *flt_ctx, SubToFrameContext *sub_to_frame_ctx)
{
  int got_sub_ptr, ret = 0;

  if ((ret = avcodec_decode_subtitle2(in_ctx->s_dec_ctx, in_ctx->dec_sub,
    &got_sub_ptr, in_ctx->init_pkt)) < 0)
  {
    fprintf(stderr, "Failed to decode subtitle.\n");
    return ret;
  }

  in_ctx->dec_sub->pts = in_ctx->init_pkt->pts;

  if ((ret = sub_to_frame_convert(sub_to_frame_ctx, in_ctx)))
  {
    fprintf(stderr, "Failed to convert subtitle to frame.\n");
    return ret;
  }

  if ((ret = filter_encode_frame(in_ctx, out_ctx,
    flt_ctx, flt_ctx->s_buffersrc_ctx, sub_to_frame_ctx->subtitle_frame)) < 0)
  {
    fprintf(stderr, "Failed to filter and encode frame.\n");
    return ret;
  }

  return 0;
}

int decode_packet(InputContext *in_ctx, OutputContext *out_ctx,
  AVStream *out_stream, FilterContext *flt_ctx,
  SubToFrameContext *sub_to_frame_ctx)
{
  int ret = 0;

  if (!in_ctx->init_pkt ||
    in_ctx->init_pkt->stream_index == in_ctx->v_stream_idx)
  {
    if ((ret =
      decode_video_packet(in_ctx, out_ctx, flt_ctx, sub_to_frame_ctx)) < 0)
    {
      fprintf(stderr, "Failed to decode video packet.\n");
      return ret;
    }
  } else if (in_ctx->init_pkt->stream_index == in_ctx->s_stream_idx)
  {
    if ((ret =
      decode_subtitle_packet(in_ctx, out_ctx, flt_ctx, sub_to_frame_ctx)) < 0)
    {
      fprintf(stderr, "Failed to decode video packet.\n");
      return ret;
    }
  } else {
    fprintf(stderr, "Recieved invalid packet in decode_packet.\n");
    return 0;
  }

  return 0;
}

int transcode(InputContext *in_ctx, OutputContext *out_ctx,
  AVStream *out_stream, FilterContext *flt_ctx,
  SubToFrameContext *sub_to_frame_ctx)
{
  int ret = 0;

  while ((ret = av_read_frame(in_ctx->fmt_ctx, in_ctx->init_pkt)) >= 0)
  {
    if (in_ctx->init_pkt->stream_index != in_ctx->v_stream_idx &&
      in_ctx->init_pkt->stream_index != in_ctx->s_stream_idx)
    {
      av_packet_unref(in_ctx->init_pkt);
      continue;
    }

    if ((ret = decode_packet(in_ctx, out_ctx, out_stream,
      flt_ctx, sub_to_frame_ctx)) < 0)
    {
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
  int v_stream_idx, s_stream_idx, ret = 0;
  char *in_filename, *out_filename, *filter_descr,
    *v_stream_idx_str, *s_stream_idx_str,
    *codec, *enc_params = NULL, *enc_params_opt = NULL;
  FilterContext *flt_ctx = NULL;
  SubToFrameContext *sub_to_frame_ctx = NULL;
  InputContext *in_ctx = NULL;
  OutputContext *out_ctx = NULL;
  AVStream *out_stream;

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
  v_stream_idx_str = argv[4];
  s_stream_idx_str = argv[5];
  codec = argv[6];

  // if ((v_stream_idx = atoi(v_stream_idx_str)) == NULL) {
  //   fprintf(stderr, "Invalid value entered for video stream.\n");
  //   goto end;
  // }
  v_stream_idx = atoi(v_stream_idx_str);

  // if ((s_stream_idx = atoi(s_stream_idx_str)) == NULL) {
  //   fprintf(stderr, "Invalid value entered for subtitle stream.\n");
  //   goto end;
  // }
  s_stream_idx = atoi(s_stream_idx_str);

  if (argc == 8) {
    enc_params = argv[7];

    if ((ret = initialize_encoder_params(codec, &enc_params_opt)) < 0) {
      fprintf(stderr, "Failed to initialize encoder option params.\n");
      return -1;
    }
  }

  if (!(in_ctx = open_input(in_filename, v_stream_idx, s_stream_idx))) {
    fprintf(stderr, "Failed to open input file: '%s'.\n", in_filename);
    goto end;
  }

  if (!(out_ctx =
    open_output(in_ctx, codec, enc_params, enc_params_opt, out_filename)))
  {
    fprintf(stderr, "Failed to open output file: '%s'.\n", out_filename);
    goto end;
  }

  out_stream = out_ctx->fmt_ctx->streams[0];

  if (!(flt_ctx = filter_context_init(in_ctx, filter_descr))) {
    fprintf(stderr, "Failed to initialize filter context.\n");
    goto end;
  }

  if (!(sub_to_frame_ctx = sub_to_frame_context_alloc(in_ctx))) {
    fprintf(stderr,
      "Failed to initialize subtitle to frame converter context.\n");
    goto end;
  }

  if ((ret =
    transcode(in_ctx, out_ctx, out_stream, flt_ctx, sub_to_frame_ctx)) < 0)
  {
    fprintf(stderr, "Failed to transcode.\n");
    goto end;
  }

  printf("flushing decoder\n");
  in_ctx->init_pkt = NULL;
  if ((ret = decode_packet(in_ctx,
    out_ctx, out_stream, flt_ctx, sub_to_frame_ctx)) < 0)
  {
    fprintf(stderr, "Failed to flush decoder.\n");
    goto end;
  }

  printf("flushing filter.\n");
  if ((ret = filter_encode_frame(in_ctx, out_ctx,
    flt_ctx, flt_ctx->v_buffersrc_ctx, NULL)) < 0)
  {
    fprintf(stderr, "Failed to flush filter.\n");
    goto end;
  }

  if ((ret = filter_encode_frame(in_ctx, out_ctx,
    flt_ctx, flt_ctx->s_buffersrc_ctx, NULL)) < 0)
  {
    fprintf(stderr, "Failed to flush filter.\n");
    goto end;
  }

  printf("flushing encoder\n");
  flt_ctx->filtered_frame = NULL;
  if ((ret = encode_frame(in_ctx, out_ctx, flt_ctx)) < 0) {
    fprintf(stderr, "Failed to flush encoder.\n");
    goto end;
  }

  printf("done flushing\n");

  if ((ret = av_write_trailer(out_ctx->fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  close_input(in_ctx);
  close_output(out_ctx);
  filter_context_free(flt_ctx);
  sub_to_frame_context_free(sub_to_frame_ctx);

  if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return ret;
  }

  return 0;
}
