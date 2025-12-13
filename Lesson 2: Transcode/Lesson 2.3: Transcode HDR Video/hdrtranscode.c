#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>

#include <libavutil/mastering_display_metadata.h>
#include <libavutil/dovi_meta.h>

typedef struct HdrMetadataContext {
  AVMasteringDisplayMetadata *mdm;
  AVContentLightMetadata *cll;
  AVDOVIMetadata *dovi;
} HdrMetadataContext;

HdrMetadataContext *hdr_metadata_alloc()
{
  HdrMetadataContext *hdr_ctx = malloc(sizeof(HdrMetadataContext));
  if (!hdr_ctx) return NULL;
  hdr_ctx->mdm = NULL;
  hdr_ctx->cll = NULL;
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
          memcpy(hdr_ctx->mdm, frame_sd->data, sizeof(AVMasteringDisplayMetadata));
        }
      }

      if (!hdr_ctx->cll) {
        frame_sd = av_frame_get_side_data(frame,
          AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);

        if (frame_sd) {
          hdr_ctx->cll = av_content_light_metadata_alloc(NULL);
          memcpy(hdr_ctx->cll, frame_sd->data, sizeof(AVContentLightMetadata));
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

    if (hdr_ctx->mdm && hdr_ctx->cll && hdr_ctx->dovi) break;
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

int inject_hdr_metadta(HdrMetadataContext *hdr_ctx, AVCodecContext *enc_ctx)
{
  int ret;

  if (hdr_ctx->mdm && hdr_ctx->cll)
  {
    if (!av_packet_side_data_add(&enc_ctx->coded_side_data,
      &enc_ctx->nb_coded_side_data, AV_PKT_DATA_MASTERING_DISPLAY_METADATA,
      (uint8_t *) hdr_ctx->mdm, sizeof(AVMasteringDisplayMetadata), 0))
    {
      fprintf(stderr, "Failed to add mastering display metadata to encoder context.\n");
      return AVERROR_UNKNOWN;
    }

    if (!av_packet_side_data_add(&enc_ctx->coded_side_data,
      &enc_ctx->nb_coded_side_data, AV_PKT_DATA_CONTENT_LIGHT_LEVEL,
      (uint8_t *) hdr_ctx->cll, sizeof(AVContentLightMetadata), 0))
    {
      fprintf(stderr, "Failed to add content light level metadata to encoder context.\n");
      return AVERROR_UNKNOWN;
    }

    if (hdr_ctx->dovi) {
      if ((ret = av_opt_set(enc_ctx->priv_data, "dolbyvision", "true", 0)) < 0) {
        fprintf(stderr, "Failed to set dolbyvision opt.\n");
        return ret;
      }
    }
  }
  return 0;
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

int main(int argc, char **argv)
{
  char *in_filename, *out_filename, *encoder,
    *enc_params = NULL, *enc_params_opt = NULL;
  int ret = 0, v_stream_idx = -1;
  HdrMetadataContext *hdr_ctx = NULL;
  AVFormatContext *in_fmt_ctx = NULL, *out_fmt_ctx = NULL;
  AVStream *in_stream = NULL, *out_stream = NULL;
  const AVCodec *dec, *enc;
  AVCodecContext *dec_ctx = NULL, *enc_ctx = NULL;
  AVPacket *pkt = NULL;
  AVFrame *frame = NULL;

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

  if (argc == 5) {
    enc_params = argv[4];

    if ((ret = initialize_encoder_params(encoder, &enc_params_opt)) < 0) {
      fprintf(stderr, "Failed to initialize encoder option params.\n");
      return -1;
    }
  }

  if (!(hdr_ctx = hdr_metadata_alloc())) {
    fprintf(stderr, "Failed to allocate HdrMetadataContext.\n");
    ret = ENOMEM;
    goto end;
  }

  if ((ret = extract_hdr_metadata(hdr_ctx, in_filename)) < 0) {
    fprintf(stderr, "Failed to get hdr metatdata from input file.\n");
    goto end;
  }

  if ((ret = avformat_open_input(&in_fmt_ctx, in_filename, NULL, NULL)) < 0) {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", in_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(in_fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  if ((ret = v_stream_idx =
    av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0)
  {
    fprintf(stderr, "Failed to find video stream in input file.\n");
    goto end;
  }
  in_stream = in_fmt_ctx->streams[v_stream_idx];

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

  if (!(enc = avcodec_find_encoder_by_name(encoder))) {
    fprintf(stderr, "Failed to find encoder.\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }

  if (!(enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder context.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  enc_ctx->time_base = in_stream->time_base;
  enc_ctx->framerate = av_guess_frame_rate(in_fmt_ctx, in_stream, NULL);

  enc_ctx->width = in_stream->codecpar->width;
  enc_ctx->height = in_stream->codecpar->height;
  enc_ctx->pix_fmt = in_stream->codecpar->format;

  enc_ctx->color_primaries = in_stream->codecpar->color_primaries;
  enc_ctx->color_trc = in_stream->codecpar->color_trc;
  enc_ctx->colorspace = in_stream->codecpar->color_space;
  enc_ctx->color_range = in_stream->codecpar->color_range;
  enc_ctx->chroma_sample_location = in_stream->codecpar->chroma_location;

  enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (enc_params && enc_params_opt) {
    if ((ret = av_opt_set(enc_ctx->priv_data,
      enc_params_opt, enc_params, 0)) < 0)
    {
      fprintf(stderr, "Failed to set %s\n", enc_params_opt);
      goto end;
    }
  }

  if ((ret = inject_hdr_metadta(hdr_ctx, enc_ctx)) < 0) {
    fprintf(stderr, "Failed to inject hdr metadata.\n");
    goto end;
  }

  if ((ret = avcodec_open2(enc_ctx, enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    goto end;
  }

  if ((ret = avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, out_filename))) {
    fprintf(stderr, "Failed to allocate output format context.\n");
    goto end;
  }

  if ((ret = av_opt_set(out_fmt_ctx, "strict", "unofficial", 0)) < 0) {
    fprintf(stderr, "Failed to set -strict unofficial.\n");
    goto end;
  }

  if ((ret = av_dict_copy(&out_fmt_ctx->metadata,
    in_fmt_ctx->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy file metadata.\n");
    goto end;
  }

  if (!(out_stream = avformat_new_stream(out_fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate new output stream.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    in_stream->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy video metadata.\n");
    goto end;
  }

  if ((ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx))) {
    fprintf(stderr,
      "Failed to copy codec parameters from encoder context to stream.\n");
    goto end;
  }

  out_stream->time_base = enc_ctx->time_base;
  out_stream->r_frame_rate = in_stream->r_frame_rate;
  out_stream->avg_frame_rate = in_stream->avg_frame_rate;

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

  if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if ((ret = avio_open(&out_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0) {
      fprintf(stderr, "Failed to open output file.\n");
      goto end;
    }
  }

  if ((ret = avformat_write_header(out_fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to write header for output file.\n");
    goto end;
  }

  while ((ret = av_read_frame(in_fmt_ctx, pkt)) >= 0)
  {
    if (!(pkt->stream_index == v_stream_idx)) {
      av_packet_unref(pkt);
      continue;
    }

    if ((ret = avcodec_send_packet(dec_ctx, pkt)) < 0) {
      fprintf(stderr, "Failed to send packet to decoder.\n");
      goto end;
    }

    while ((ret = avcodec_receive_frame(dec_ctx, frame)) >= 0)
    {
      if ((ret = avcodec_send_frame(enc_ctx, frame)) < 0) {
        fprintf(stderr, "Failed to send frame to encoder.\n");
        goto end;
      }

      while ((ret = avcodec_receive_packet(enc_ctx, pkt)) >= 0)
      {
        pkt->stream_index = 0;
        av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);

        if ((ret = av_interleaved_write_frame(out_fmt_ctx, pkt)) < 0) {
          fprintf(stderr, "Failed to write packet to file.\n");
          goto end;
        }
        av_packet_unref(pkt);
      }

      if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
        fprintf(stderr, "Failed to receive packet from encoder.\n");
        goto end;
      }
    }

    if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
      fprintf(stderr, "Failed to receive frame from decoder.\n");
      goto end;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to read frame from input file.\n");
    goto end;
  }

  if ((ret = avcodec_send_frame(enc_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to send frame to encoder.\n");
    goto end;
  }

  if ((ret = av_write_trailer(out_fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  free(hdr_ctx);
  avformat_close_input(&in_fmt_ctx);
  if (out_fmt_ctx && !(out_fmt_ctx->flags & AVFMT_NOFILE))
    avio_closep(&out_fmt_ctx->pb);
  avformat_free_context(out_fmt_ctx);
  avcodec_free_context(&dec_ctx);
  avcodec_free_context(&enc_ctx);
  av_packet_free(&pkt);
  av_frame_free(&frame);

  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return -1;
  }

  return 0;
}