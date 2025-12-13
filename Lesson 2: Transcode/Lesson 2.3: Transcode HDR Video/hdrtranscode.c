#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>

#include <libavutil/mastering_display_metadata.h>
#include <libavutil/dovi_meta.h>

typedef struct HdrMetadataContext {
  AVMasteringDisplayMetadata *mdm;
  char *mdm_str;
  AVContentLightMetadata *cll;
  char *cll_str;
  AVDOVIMetadata *dovi;
} HdrMetadataContext;

static int max_len_mdm_str()
{
  char *max_mdm_str = "G(2147483647,2147483647)B(2147483647,2147483647)R(2147483647,2147483647)WP(2147483647,2147483647)L(2147483647,2147483647)";
  char *end;
  int len_max_mdm_str;

  for (end = max_mdm_str; *end; end++);
  len_max_mdm_str = end - max_mdm_str;

  return len_max_mdm_str;
}

#define MDM_STR_MAX_LEN max_len_mdm_str()

static int max_len_cll_str()
{
  char *max_cll_str = "2147483647,2147483647";
  char *end;
  int len_max_cll_str;

  for (end = max_cll_str; *end; end++);
  len_max_cll_str = end - max_cll_str;

  return len_max_cll_str;
}

#define CLL_STR_MAX_LEN max_len_cll_str()

static int len_hdr_params_str(int dovi)
{
  char *params_base, *end;
  int len_params_base, len_params;

  if (dovi) {
    params_base =
      "master-display=:max-cll=:vbv-maxrate=100000:vbv-bufsize=200000:";
  } else {
    params_base = "master-display=:max-cll=:";
  }

  for (end = params_base; *end; end++);
  len_params_base = end - params_base;

  len_params = len_params_base + MDM_STR_MAX_LEN + CLL_STR_MAX_LEN;

  return len_params;
}

#define DOVI_PARAMS_STR_LEN len_hdr_params_str(1)
#define HDR_PARAMS_STR_LEN len_hdr_params_str(0)

HdrMetadataContext *hdr_ctx_alloc()
{
  HdrMetadataContext *hdr_ctx = malloc(sizeof(HdrMetadataContext));
  if (!hdr_ctx) return NULL;
  hdr_ctx->mdm = NULL;
  hdr_ctx->mdm_str = NULL;
  hdr_ctx->cll = NULL;
  hdr_ctx->cll_str = NULL;
  hdr_ctx->dovi = NULL;
  return hdr_ctx;
}

int extract_hdr_metadata(HdrMetadataContext *hdr_ctx, const char *filename)
{
  AVFormatContext *fmt_ctx = NULL;
  AVStream *in_stream = NULL;
  AVCodecContext *dec_ctx = NULL;
  const AVCodec *dec;
  AVPacket *pkt = NULL;
  AVFrame *frame = NULL;
  int ret = 0, v_stream_idx = -1, frames_decoded = 0;
  AVFrameSideData *frame_sd;

  #define FRAME_DECODE_LIMIT 100

  if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  if ((ret = v_stream_idx =
    av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0)
  {
    fprintf(stderr, "Failed to find video stream in input file.\n");
    goto end;
  }

  in_stream = fmt_ctx->streams[v_stream_idx];

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder.\n");
    ret = AVERROR(EINVAL);
    goto end;
  }

  if (!(dec_ctx = avcodec_alloc_context3(dec))) {
    fprintf(stderr, "Failed to allocate decoder context.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = avcodec_parameters_to_context(dec_ctx, in_stream->codecpar)) < 0) {
    fprintf(stderr, "Failed to copy codec parameters to decoder context.\n");
    goto end;
  }

  if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
    fprintf(stderr, "Failed to open decoder.\n");
    goto end;
  }

  if (!(pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  while (av_read_frame(fmt_ctx, pkt) >= 0 && frames_decoded < FRAME_DECODE_LIMIT)
  {
    if (pkt->stream_index != v_stream_idx) {
      av_packet_unref(pkt);
      continue;
    }

    if ((ret = avcodec_send_packet(dec_ctx, pkt)) < 0) {
      fprintf(stderr, "Failed to send video packet to decoder.\n");
      av_packet_unref(pkt);
      goto end;
    }

    while ((ret = avcodec_receive_frame(dec_ctx, frame)) >= 0)
    {
      frames_decoded++;

      if (!hdr_ctx->mdm) {
        frame_sd = av_frame_get_side_data(frame,
          AV_FRAME_DATA_MASTERING_DISPLAY_METADATA);

        if (frame_sd) {
          hdr_ctx->mdm = av_mastering_display_metadata_alloc();
          memcpy(hdr_ctx->mdm, frame_sd->data,
            sizeof(AVMasteringDisplayMetadata));

          if (!(hdr_ctx->mdm_str =
            calloc(MDM_STR_MAX_LEN + 1, sizeof(char))))
          {
            fprintf(stderr, "Failed to allocate hdr_ctx->mdm_str\n");
            ret = AVERROR(ENOMEM);
            goto end;
          }

          snprintf(hdr_ctx->mdm_str,
            MDM_STR_MAX_LEN + 1,
            "G(%d,%d)B(%d,%d)R(%d,%d)WP(%d,%d)L(%d,%d)",
            hdr_ctx->mdm->display_primaries[1][0].num,
            hdr_ctx->mdm->display_primaries[1][1].num,
            hdr_ctx->mdm->display_primaries[2][0].num,
            hdr_ctx->mdm->display_primaries[2][1].num,
            hdr_ctx->mdm->display_primaries[0][0].num,
            hdr_ctx->mdm->display_primaries[0][1].num,
            hdr_ctx->mdm->white_point[0].num,
            hdr_ctx->mdm->white_point[1].num,
            hdr_ctx->mdm->max_luminance.num,
            hdr_ctx->mdm->min_luminance.num);
        }
      }

      if (!hdr_ctx->cll) {
        frame_sd = av_frame_get_side_data(frame,
          AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);

        if (frame_sd) {
          hdr_ctx->cll = av_content_light_metadata_alloc(NULL);
          memcpy(hdr_ctx->cll, frame_sd->data, sizeof(AVContentLightMetadata));

          if (!(hdr_ctx->cll_str =
            calloc(CLL_STR_MAX_LEN + 1, sizeof(char))))
          {
            fprintf(stderr, "Failed to allocate hdr_ctx->cll_str\n");
            ret = AVERROR(ENOMEM);
            goto end;
          }

          snprintf(hdr_ctx->cll_str, CLL_STR_MAX_LEN + 1, "%d,%d",
            hdr_ctx->cll->MaxCLL, hdr_ctx->cll->MaxFALL);
        }
      }

      if (!hdr_ctx->dovi) {
        frame_sd = av_frame_get_side_data(frame,
          AV_FRAME_DATA_DOVI_METADATA);

        if (frame_sd) {
          hdr_ctx->dovi = av_dovi_metadata_alloc(NULL);
          memcpy(hdr_ctx->dovi, frame_sd->data, sizeof(AVDOVIMetadata));
        }
      }

      av_frame_unref(frame);
      if (hdr_ctx->mdm && hdr_ctx->cll && hdr_ctx->dovi) break;
    }

    if ((ret < 0 && ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
      fprintf(stderr, "Failed to receive frame from decoder.: %d\n", ret);
      goto end;
    }

    if (hdr_ctx->mdm && hdr_ctx->cll && hdr_ctx->dovi) {
      break;
    }
  }

  if ((ret < 0 && ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to read frame from input file.\n");
    goto end;
  }

end:
  avformat_close_input(&fmt_ctx);
  avcodec_free_context(&dec_ctx);
  av_packet_free(&pkt);
  av_frame_free(&frame);

  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return 0;
  return ret;
}

int inject_hdr_metadta(HdrMetadataContext *hdr_ctx, AVCodecContext *enc_ctx,
  char **params_str)
{
  int ret;

  if (hdr_ctx->mdm && hdr_ctx->cll)
  {
    if (!av_packet_side_data_add(&enc_ctx->coded_side_data,
      &enc_ctx->nb_coded_side_data, AV_PKT_DATA_MASTERING_DISPLAY_METADATA,
      (uint8_t *) hdr_ctx->mdm, sizeof(AVMasteringDisplayMetadata), 0))
    {
      fprintf(stderr,
        "Failed to add mastering display metadata to encoder context.\n");
      return AVERROR_UNKNOWN;
    }

    if (!av_packet_side_data_add(&enc_ctx->coded_side_data,
      &enc_ctx->nb_coded_side_data, AV_PKT_DATA_CONTENT_LIGHT_LEVEL,
      (uint8_t *) hdr_ctx->cll, sizeof(AVContentLightMetadata), 0))
    {
      fprintf(stderr,
        "Failed to add content light level metadata to encoder context.\n");
      return AVERROR_UNKNOWN;
    }

    if (*params_str) {
      free(*params_str);
    }

    if (hdr_ctx->dovi) {
      if (!av_packet_side_data_add(&enc_ctx->coded_side_data,
        &enc_ctx->nb_coded_side_data, (enum AVPacketSideDataType) AV_FRAME_DATA_DOVI_METADATA,
        (uint8_t *) hdr_ctx->dovi, sizeof(AVDOVIMetadata), 0))
      {
        fprintf(stderr,
          "Failed to add content light level metadata to encoder context.\n");
        return AVERROR_UNKNOWN;
      }

      if ((ret = av_opt_set(enc_ctx->priv_data, "dolbyvision", "true", 0)) < 0) {
        fprintf(stderr, "Failed to set dolbyvision opt.\n");
        return ret;
      }

      if (!(*params_str = calloc(DOVI_PARAMS_STR_LEN + 1, sizeof(char))))
      {
        fprintf(stderr, "Failed to allocate params_str\n");
        return AVERROR(ENOMEM);
      }

      snprintf(*params_str, DOVI_PARAMS_STR_LEN,
        "master-display=%s:max-cll=%s:vbv-maxrate=100000:vbv-bufsize=200000:",
        hdr_ctx->mdm_str, hdr_ctx->cll_str);
    }
    else {
      if (!(*params_str = calloc(HDR_PARAMS_STR_LEN + 1, sizeof(char))))
      {
        fprintf(stderr, "Failed to allocate params_str\n");
        return AVERROR(ENOMEM);
      }

      snprintf(*params_str, HDR_PARAMS_STR_LEN,
        "master-display=%s:max-cll=%s:", hdr_ctx->mdm_str, hdr_ctx->cll_str);
    }
  }

  return 0;
}

void hdr_ctx_free(HdrMetadataContext *hdr_ctx)
{
  free(hdr_ctx->mdm_str);
  free(hdr_ctx->cll_str);
  free(hdr_ctx);
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

  if (stream_idx >= in_ctx->fmt_ctx->nb_streams) {
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

static int get_len_params_str(char *hdr_params_str, char *additional_params_str)
{
  char *end;
  int len_hdr_params_str = 0, len_additional_params_str = 0;

  if (additional_params_str) {
    for (end = additional_params_str; *end; end++);
    len_additional_params_str = end - additional_params_str;
  }

  if (hdr_params_str) {
    for (end = hdr_params_str; *end; end++);
    len_hdr_params_str = end - hdr_params_str;
  }

  return len_hdr_params_str + len_additional_params_str;
}

typedef struct OutputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *enc_ctx;
  AVPacket *enc_pkt;
} OutputContext;

OutputContext *open_output(InputContext *in_ctx,
  const char *codec, char *user_enc_params, char *enc_params_opt,
  char *in_filename, const char *out_filename)
{
  int ret = 0;
  OutputContext *out_ctx = NULL;
  AVStream *in_stream, *out_stream;
  const AVCodec *enc;
  HdrMetadataContext *hdr_ctx = NULL;
  char *params_str = NULL, *hdr_params_str = NULL;
  int len_params_str;

  if (!(out_ctx = malloc(sizeof(OutputContext)))) {
    fprintf(stderr, "Failed to allocate OutputContext.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  out_ctx->fmt_ctx = NULL;
  out_ctx->enc_ctx = NULL;
  out_ctx->enc_pkt = NULL;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder.\n");
    ret = AVERROR(EINVAL);
    goto end;
  }

  if (!(out_ctx->enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder.\n");
    ret = AVERROR(EINVAL);
    goto end;
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

  if (!(hdr_ctx = hdr_ctx_alloc())) {
    fprintf(stderr, "Failed to allocate hdr metada.\n");
    ret = -ENOMEM;
    goto end;
  }

  if ((ret = extract_hdr_metadata(hdr_ctx, in_filename)) < 0) {
    fprintf(stderr, "Failed to extract hdr metadata.\n");
    goto end;
  }

  if ((ret = inject_hdr_metadta(hdr_ctx, out_ctx->enc_ctx, &hdr_params_str)) < 0) {
    fprintf(stderr, "Failed to inject hdr metadata.\n");
    goto end;
  }

  len_params_str = get_len_params_str(hdr_params_str, user_enc_params);

  if (!(params_str = calloc(len_params_str + 1, sizeof(char))))
  {
    fprintf(stderr, "Failed to allocate params_str\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (hdr_params_str) { strcat(params_str, hdr_params_str); }
  if (user_enc_params) { strcat(params_str, user_enc_params); }

  printf("params_str: %s\n", params_str);
  printf("enc_params_opt: %s\n", enc_params_opt);

  if ((ret = av_opt_set(out_ctx->enc_ctx->priv_data, enc_params_opt,
    params_str, 0)) < 0)
  {
    fprintf(stderr, "Failed to set x265-params.\n");
    goto end;
  }

  if ((ret = avcodec_open2(out_ctx->enc_ctx, enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    goto end;
  }

  if ((ret =
    avformat_alloc_output_context2(&out_ctx->fmt_ctx, NULL, NULL, out_filename)))
  {
    fprintf(stderr,
      "Failed to allocate output format context.\n");
    goto end;
  }

  if ((ret = av_dict_copy(&out_ctx->fmt_ctx->metadata, in_ctx->fmt_ctx->metadata,
    AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr,
      "Failed to copy input metadata to output.\n");
    goto end;
  }

  if (!(out_stream = avformat_new_stream(out_ctx->fmt_ctx, NULL))) {
    fprintf(stderr,
      "Failed to allocate new output stream.\n");
    goto end;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    in_stream->metadata, AV_DICT_DONT_OVERWRITE)))
  {
    fprintf(stderr,
      "Failed to copy metadata from input stream to output stream.\n");
    goto end;
  }

  if ((ret =
    avcodec_parameters_from_context(out_stream->codecpar, out_ctx->enc_ctx)))
  {
    fprintf(stderr,
      "Failed to copy parameters from encoder to output stream.\n");
    goto end;
  }

  out_stream->time_base = out_ctx->enc_ctx->time_base;
  out_stream->r_frame_rate = in_stream->r_frame_rate;
  out_stream->avg_frame_rate = in_stream->avg_frame_rate;

  if (!(out_ctx->enc_pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(out_ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if ((ret =
      avio_open(&out_ctx->fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0)
    {
      fprintf(stderr, "Failed to create output file.\n");
      goto end;
    }
  }

  if ((ret = avformat_write_header(out_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to write header to output.\n");
    goto end;
  }

end:
  hdr_ctx_free(hdr_ctx);
  free(hdr_params_str);
  free(params_str);

  if (ret < 0) return NULL;
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

int encode_frame(InputContext *in_ctx, AVStream *in_stream,
  OutputContext *out_ctx, AVStream *out_stream)
{
  int ret = 0;

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
  OutputContext *out_ctx, AVStream *out_stream)
{
  int ret = 0;

  if ((ret = avcodec_send_packet(in_ctx->dec_ctx, in_ctx->init_pkt)) < 0) {
    fprintf(stderr, "Failed to send packet to decoder.\n");
    return ret;
  }

  while ((ret = avcodec_receive_frame(in_ctx->dec_ctx, in_ctx->dec_frame)) >= 0)
  {
    if ((ret = encode_frame(in_ctx, in_stream, out_ctx, out_stream)) < 0) {
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
  OutputContext *out_ctx, AVStream *out_stream)
{
  int ret = 0;

  while ((ret = av_read_frame(in_ctx->fmt_ctx, in_ctx->init_pkt)) >= 0)
  {
    if (!(in_ctx->init_pkt->stream_index == in_ctx->stream_idx)) {
      av_packet_unref(in_ctx->init_pkt);
      continue;
    }

    if ((ret = decode_packet(in_ctx, in_stream, out_ctx, out_stream)) < 0) {
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
  char *in_filename, *out_filename, *encoder,
    *enc_params = NULL, *enc_params_opt = NULL;
  InputContext *in_ctx = NULL;
  OutputContext *out_ctx = NULL;
  AVStream *in_stream = NULL, *out_stream = NULL;

  if (argc != 4 && argc != 5) {
    printf("\nUsage: %s <input file> <output file> <encoder> [<encoder-params>]\n\n\t"
      "This example will take in a file with a video stream, transcode the video,\n\t"
      "and save it to <output file>. If the video has HDR metadata and/or\n\t"
      "Dolbyvision metadata, it will be copied over to the output file.\n\t"
      "Optionally, you can pass in a colon\n\t"
      "seperated string with parameters that will be passed to the encoder.\n\t"
      "encoder-params is supported for libx264, libx265, and libsvtav1.\n\n",
      argv[0]);
    return 0;
  }

  in_filename = argv[1];
  out_filename = argv[2];
  encoder = argv[3];
  enc_params = argv[4];

  if ((ret = initialize_encoder_params(encoder, &enc_params_opt)) < 0) {
    fprintf(stderr, "Failed to initialize encoder option params.\n");
    return -1;
  }

  if (!(in_ctx = open_input(in_filename, 0))) {
    fprintf(stderr, "Failed to open input file: '%s'.\n", in_filename);
    goto end;
  }

  in_stream = in_ctx->fmt_ctx->streams[in_ctx->stream_idx];

  if (!(out_ctx =
    open_output(in_ctx, encoder, enc_params, enc_params_opt, in_filename, out_filename)))
  {
    fprintf(stderr, "Failed to open output file: '%s'.\n", out_filename);
    goto end;
  }

  out_stream = out_ctx->fmt_ctx->streams[0];

  if ((ret = transcode(in_ctx, in_stream, out_ctx, out_stream)) < 0) {
    fprintf(stderr, "Failed to transcode.\n");
    goto end;
  }

  in_ctx->init_pkt = NULL;
  if ((ret = decode_packet(in_ctx, in_stream, out_ctx, out_stream)) < 0) {
    fprintf(stderr, "Failed to flush decoder.\n");
    goto end;
  }

  in_ctx->dec_frame = NULL;
  if ((ret = encode_frame(in_ctx, in_stream, out_ctx, out_stream)) < 0) {
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

  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return -1;
  }

  return 0;
}
