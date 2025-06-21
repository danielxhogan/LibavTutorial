#include <libavformat/avformat.h>

int initialize_stream(AVFormatContext *out_fmt_ctx, AVStream *in_stream)
{
  AVStream *out_stream;
  int ret = 0;

  if (!(out_stream = avformat_new_stream(out_fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate output stream for input stream.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  if ((ret = avcodec_parameters_copy(out_stream->codecpar,
    in_stream->codecpar)) < 0)
  {
    fprintf(stderr, "Failed to copy codec parameters for input stream.\n");
    return ret;
  }
  out_stream->codecpar->codec_tag = 0;

  if ((out_stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) &&
    !(strcmp(out_fmt_ctx->oformat->name, "mp4")))
  {
    out_stream->codecpar->codec_id = AV_CODEC_ID_MOV_TEXT;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    in_stream->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy metadata for input stream.\n");
    return ret;
  }
  return ret;
}

int main(int argc, char **argv)
{
  const char *in_filename, *out_filename;
  int start_second, duration, ret;
  int64_t start_ts, end_ts, *dts_starts, *pts_starts;
  AVFormatContext *in_fmt_ctx = NULL, *out_fmt_ctx = NULL;
  AVPacket *pkt = NULL;
  AVStream *in_stream = NULL, *out_stream = NULL;

  if (argc != 5) {
    printf("\nUsage: %s <seek time> <input file> <duration> <output file>\n",
      argv[0]);
    return 0;
  }

  start_second = strtod(argv[1], NULL);
  in_filename = argv[2];
  duration = strtod(argv[3], NULL);
  out_filename = argv[4];

  if ((ret = avformat_open_input(&in_fmt_ctx, in_filename, NULL, NULL)) < 0) {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", in_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(in_fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  if ((ret =
    avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, out_filename)))
  {
    fprintf(stderr, "Failed to allocate output format context.\n");
    goto end;
  }

  for (int i = 0; i < in_fmt_ctx->nb_streams; i++) {
    if ((ret = initialize_stream(out_fmt_ctx, in_fmt_ctx->streams[i])) < 0) {
      fprintf(stderr, "Failed to initialize stream %d.\n", i);
      goto end;
    }
  }

  start_ts = (int64_t) (start_second * AV_TIME_BASE);
  end_ts = (int64_t) ((start_second + duration) * AV_TIME_BASE);

  if ((ret = av_seek_frame(in_fmt_ctx, -1, start_ts, AVSEEK_FLAG_ANY)) < 0) {
    fprintf(stderr, "Failed to seek to start frame.\n");
    goto end;
  }

  dts_starts = av_calloc(in_fmt_ctx->nb_streams, sizeof(int64_t));
  pts_starts = av_calloc(in_fmt_ctx->nb_streams, sizeof(int64_t));
  if (!dts_starts || !pts_starts) {
    fprintf(stderr, "Failed to allocate start timestamp arrays.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  for (int i = 0; i < in_fmt_ctx->nb_streams; i++) {
    dts_starts[i] = AV_NOPTS_VALUE;
    pts_starts[i] = AV_NOPTS_VALUE;
  }

  if (!(pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if ((ret =
      avio_open(&out_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0)
    {
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
    // if (pkt->stream_index != 0) continue;
    printf("here: %ld\n", pkt->pts);
    in_stream = in_fmt_ctx->streams[pkt->stream_index];
    out_stream = out_fmt_ctx->streams[pkt->stream_index];

    int64_t pkt_pts_avtb = av_rescale_q(pkt->pts, in_stream->time_base, AV_TIME_BASE_Q);
    int64_t pkt_dts_avtb = av_rescale_q(pkt->dts, in_stream->time_base, AV_TIME_BASE_Q);

    // if (pkt_dts_avtb < start_ts) {
    //   av_packet_unref(pkt);
    //   continue;
    // }

    if (pkt_dts_avtb >= end_ts) {
      av_packet_unref(pkt);
      break;
    }

    if (dts_starts[pkt->stream_index] == AV_NOPTS_VALUE) {
      dts_starts[pkt->stream_index] = pkt->dts;
      pts_starts[pkt->stream_index] = pkt->pts;
    }

    pkt->pts = av_rescale_q(pkt->pts - pts_starts[pkt->stream_index],
      in_stream->time_base, out_stream->time_base);

    pkt->dts = av_rescale_q(pkt->dts - dts_starts[pkt->stream_index],
      in_stream->time_base, out_stream->time_base);

    pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);

    // pkt->pts = av_rescale_q(pkt->pts,
    //   in_stream->time_base, out_stream->time_base);

    // pkt->dts = av_rescale_q(pkt->dts,
    //   in_stream->time_base, out_stream->time_base);

    // pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);

    if ((ret = av_interleaved_write_frame(out_fmt_ctx, pkt)) < 0) {
      fprintf(stderr, "Failed to write packet to file.\n");
      goto end;
    }

    av_packet_unref(pkt);
  }

  if ((ret = av_write_trailer(out_fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  avformat_close_input(&in_fmt_ctx);

  if (out_fmt_ctx && !(out_fmt_ctx->flags & AVFMT_NOFILE))
    avio_closep(&out_fmt_ctx->pb);
  avformat_free_context(out_fmt_ctx);

  av_packet_free(&pkt);

  if (ret < 0 && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return -1;
  }

  return 0;
}