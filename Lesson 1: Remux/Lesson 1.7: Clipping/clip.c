/*
 * This function takes in a video file along with a timestamp in seconds and a
 * duration. It seeks to the closest I frame to the timestamp and reads however
 * many seconds specified by the duration from the input and outputs it to a
 * file specified by the output argument. It is roughly the equivalent to the
 * following ffmpeg commad:
 *
 * ffmpeg \
 *   -ss <seek time> \
 *   -i <input file> \
 *   -t <duration> \
 *   -map 0 -c copy \
 *   <output file>
*/

#include <libavformat/avformat.h>

enum FIRST_DTS_SET {
  NOT_SET,
  SET
};

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
  int start_sec, duration_sec, video_idx = -1, ret;
  int64_t start_ts, duration_ts, end_ts, first_dts;
  enum FIRST_DTS_SET first_dts_set = NOT_SET;
  AVFormatContext *in_fmt_ctx = NULL, *out_fmt_ctx = NULL;
  AVPacket *pkt = NULL;
  AVStream *in_stream = NULL, *out_stream = NULL;

  if (argc != 5) {
    printf("\nUsage: %s <seek time> <input file> <duration> <output file>\n\n\t"
      "This function takes in a video file along with a timestamp in seconds\n\t"
      "and a duration. It seeks to the closest I frame to the timestamp\n\t"
      "and reads however many seconds specified by the duration from the\n\t"
      "input file and outputs it to a file specified by output file. It is\n\t"
      "roughly the equivalent of the following ffmpeg commad:\n\n\t"
      "ffmpeg -ss <seek time> -i <input file> -t <duration> "
      "-map 0 -c copy <output file>\n\n",
      argv[0]);
    return 0;
  }

  start_sec = strtod(argv[1], NULL);
  in_filename = argv[2];
  duration_sec = strtod(argv[3], NULL);
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

    if (out_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_idx = i;
    }
  }

  if (video_idx == -1) {
    fprintf(stderr, "Failed to find video stream.\n");
    goto end;
  }

  start_ts =
    start_sec *
    in_fmt_ctx->streams[video_idx]->time_base.den /
    in_fmt_ctx->streams[video_idx]->time_base.num;

  if ((ret =
    av_seek_frame(in_fmt_ctx, video_idx, start_ts, AVSEEK_FLAG_BACKWARD)) < 0)
  {
    fprintf(stderr, "Failed to seek to start frame.\n");
    goto end;
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
    if (
      pkt->dts < 0 ||
      pkt->pts < 0
    ) {
        av_packet_unref(pkt);
        continue;
      }

    in_stream = in_fmt_ctx->streams[pkt->stream_index];
    out_stream = out_fmt_ctx->streams[pkt->stream_index];

    if (first_dts_set == NOT_SET)
    {
      printf("have not found first keyframe. Current frame type: %d\n",
        in_stream->codecpar->codec_type);

      if (pkt->stream_index == video_idx)
      {
        if (!(pkt->flags & AV_PKT_FLAG_KEY)) {
          av_packet_unref(pkt);
          continue;
        }
        printf("Current frame is first keyframe.\n");

        first_dts = pkt->dts;
        first_dts_set = SET;
        duration_ts = av_rescale_q(duration_sec * AV_TIME_BASE, AV_TIME_BASE_Q,
          in_fmt_ctx->streams[video_idx]->time_base);
        end_ts = first_dts + duration_ts;

        printf("first_dts: %ld\n", first_dts);
        printf("duration_ts: %ld\n", duration_ts);
        printf("end_ts: %ld\n", end_ts);
      }
      else {
        av_packet_unref(pkt);
        continue;
      }
    }

    if (end_ts && pkt->dts > end_ts) {
      av_packet_unref(pkt);
      break;
    }

    // printf("before rescaling:\n");
    // printf("pkt->pts: %ld\n", pkt->pts);
    // printf("pkt->dts: %ld\n", pkt->dts);

    pkt->pts = av_rescale_q(pkt->pts - first_dts,
      in_stream->time_base, out_stream->time_base);

    pkt->dts = av_rescale_q(pkt->dts - first_dts,
      in_stream->time_base, out_stream->time_base);

    pkt->duration = av_rescale_q(pkt->duration,
      in_stream->time_base,out_stream->time_base);

    // printf("after rescaling:\n");
    // printf("pkt->pts: %ld\n", pkt->pts);
    // printf("pkt->dts: %ld\n", pkt->dts);

    pkt->pos = -1;
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