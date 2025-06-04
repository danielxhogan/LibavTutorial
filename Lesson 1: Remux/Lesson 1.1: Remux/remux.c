/*
 * This program takes in a video file and reformats the file using the output
 * file name specified by the user to determine the format. It will not decode
 * any data. It will assume the video has at least one video stream and one
 * audio stream and will ignore all other streams. It will be roughly the
 * equivalent of the following ffmpeg command:
 *
 * ffmpeg -i <input file> -c copy <output file>
 * 
 */

#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>

int main(int argc, char **argv)
{
  const char *in_filename, *out_filename;
  AVFormatContext *in_fmt_ctx = NULL, *out_fmt_ctx = NULL;
  int v_stream_idx, a_stream_idx;
  AVStream *in_stream, *out_stream;
  AVPacket *pkt = NULL;
  int ret;

  if (argc != 3) {
    printf("\nUsage: %s <input file> <output file>\n\n\t"
      "This program will take in a video file and reformat the file using\n\t"
      "the output file name specified by the user to determine the format.\n\t"
      "It will not decode any data. It will assume the video has at least\n\t"
      "one video stream and one audio stream and will ignore all other\n\t"
      "streams. It will be roughly the equivalent of the following\n\t"
      "ffmpeg command:\n\n\t"
      "ffmpeg -i <input file> -c copy <output file>\n\n"
      , argv[0]);
    return 0;
  }

  in_filename = argv[1];
  out_filename = argv[2];

  if ((ret = avformat_open_input(&in_fmt_ctx, in_filename, NULL, NULL)) < 0) {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", in_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(in_fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  av_dump_format(in_fmt_ctx, 0, in_filename, 0);

  if ((ret =
    avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, out_filename)))
  {
    fprintf(stderr, "Failed to allocate output format context.\n");
    goto end;
  }

  if ((ret = v_stream_idx =
    av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0)
  {
    fprintf(stderr, "Failed to find video stream in input file.\n");
    goto end;
  }

  if (!(out_stream = avformat_new_stream(out_fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate video output stream.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = avcodec_parameters_copy(out_stream->codecpar,
    in_fmt_ctx->streams[v_stream_idx]->codecpar)) < 0)
  {
    fprintf(stderr, "Failed to copy video codec parameters\n");
    goto end;
  }
  out_stream->codecpar->codec_tag = 0;

  if ((ret = a_stream_idx =
    av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0)) < 0)
  {
    fprintf(stderr, "Failed to find audio stream in input file.\n");
    goto end;
  }

  if (!(out_stream = avformat_new_stream(out_fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate audio output stream.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = avcodec_parameters_copy(out_stream->codecpar,
    in_fmt_ctx->streams[a_stream_idx]->codecpar)) < 0)
  {
    fprintf(stderr, "Failed to copy audio codec parameters\n");
    goto end;
  }
  out_stream->codecpar->codec_tag = 0;

  av_dump_format(out_fmt_ctx, 0, out_filename, 1);

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
    in_stream = in_fmt_ctx->streams[pkt->stream_index];

    if (pkt->stream_index == v_stream_idx) {
      pkt->stream_index = 0;
    }
    else if (pkt->stream_index == a_stream_idx) {
      pkt->stream_index = 1;
    }
    else {
      av_packet_unref(pkt);
      continue;
    }

    pkt->pos = -1;
    out_stream = out_fmt_ctx->streams[pkt->stream_index];
    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);

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
