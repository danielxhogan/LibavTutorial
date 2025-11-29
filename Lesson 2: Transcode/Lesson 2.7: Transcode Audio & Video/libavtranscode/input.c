#include "input.h"

static int open_decoder(InputContext *in_ctx,
  AVStream *in_stream, int in_stream_idx)
{
  int ret = 0;
  const AVCodec *dec;

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder.\n");
    ret = AVERROR(EINVAL);
    return ret;
  }

  if (!(in_ctx->dec_ctx[in_stream_idx] = avcodec_alloc_context3(dec))) {
    fprintf(stderr, "Failed to allocate decoder context "
      "for in_stream_idx: %d.\n", in_stream->index);
    ret = AVERROR(ENOMEM);
    return ret;
  }

  if ((ret =
    avcodec_parameters_to_context(in_ctx->dec_ctx[in_stream_idx],
      in_stream->codecpar)) < 0)
  {
    fprintf(stderr, "Failed to copy codec parameters to decoder context "
      "for in_stream_idx: %d.\n", in_stream->index);
    return ret;
  }

  if ((ret =
    avcodec_open2(in_ctx->dec_ctx[in_stream_idx], dec, NULL)) < 0)
  {
    fprintf(stderr, "Failed to open decoder for in_stream_idx: %d.\n",
      in_stream->index);
    return ret;
  }

  return ret;
}

InputContext *open_input(const char *in_filename, const char *selected_streams)
{
  int ret = 0, i, in_stream_idx;
  AVStream *in_stream;

  InputContext *in_ctx = malloc(sizeof(InputContext));
  if (!in_ctx) {
    ret = ENOMEM;
    goto end;
  }

  in_ctx->fmt_ctx = NULL;
  in_ctx->dec_ctx = NULL;
  in_ctx->dec_frame = NULL;
  in_ctx->map = NULL;

  for (i = 0; selected_streams[i] != '\0'; i++);
  in_ctx->nb_selected_streams = i;

  if ((ret =
    avformat_open_input(&in_ctx->fmt_ctx, in_filename, NULL, NULL)) < 0)
  {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", in_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(in_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  if (!(in_ctx->map =
    calloc(in_ctx->fmt_ctx->nb_streams, sizeof(int))))
  {
    fprintf(stderr, "Failed to allocate map array.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  for (i = 0; i < in_ctx->fmt_ctx->nb_streams; i++) {
    in_ctx->map[i] = INACTIVE_STREAM;
  }

  if (!(in_ctx->dec_ctx =
    calloc(in_ctx->fmt_ctx->nb_streams, sizeof(AVCodecContext *))))
  {
    fprintf(stderr, "Failed to allocate in_ctx->dec_ctx array.\n");
    goto end;
  }

  for (i = 0; i < in_ctx->nb_selected_streams; i++)
  {
    in_stream_idx = selected_streams[i] - '0';
    if (in_stream_idx < 0 || in_stream_idx > 9) {
      fprintf(stderr,
        "Invalid character found when parsing selected_streams.\n");
      return 0;
    }

    if (in_stream_idx >= in_ctx->fmt_ctx->nb_streams) {
      fprintf(stderr, "Stream index '%d' does not exist.\n", in_stream_idx);
      return 0;
    }

    in_ctx->map[in_stream_idx] = i;

    in_stream = in_ctx->fmt_ctx->streams[in_stream_idx];

    if (in_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ||
      in_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      if ((ret = open_decoder(in_ctx, in_stream, in_stream_idx)) < 0) {
        fprintf(stderr, "Failed to open decoder for stream %d\n",
          in_stream_idx);
        goto end;
      }
    }
  }

  if (!(in_ctx->pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(in_ctx->dec_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
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

  if (in_ctx->dec_ctx) {
    for (int i = 0; i < in_ctx->nb_selected_streams; i++) {
      avcodec_free_context(&in_ctx->dec_ctx[i]);
    }
    free(in_ctx->dec_ctx);
  }

  av_packet_free(&in_ctx->pkt);
  av_frame_free(&in_ctx->dec_frame);
  free(in_ctx->map);
  free(in_ctx);
}
