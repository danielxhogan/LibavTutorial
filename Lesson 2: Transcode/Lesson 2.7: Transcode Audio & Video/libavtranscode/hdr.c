#include "hdr.h"

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
