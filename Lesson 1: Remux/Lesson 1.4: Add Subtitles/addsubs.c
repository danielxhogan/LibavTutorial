/*
 * This program performs all the same operations as cpmetadata.c. In addition,
 * it also takes in an .srt file with subtitle information in it, creates a
 * subtitle stream with the information, and embeds the stream in the output
 * file. It is roughly the equivalent of the following ffmpeg command:
 *
 * ffmpeg \
 *   -i "<video file>" \
 *   -i "<subtitle file>" \
 *   -c copy \
 *   -metadata:s:s:0 language=eng \
 *   -metadata title="<title>" \
 *   "<output file>"
 */

#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>

int copy_chapters(AVFormatContext *out_fmt_ctx, AVFormatContext *in_fmt_ctx)
{
  AVChapter *in_chapter, *out_chapter;
  int ret = 0;

  if (!(out_fmt_ctx->chapters =
    av_calloc(in_fmt_ctx->nb_chapters, sizeof(AVChapter *))))
  {
      fprintf(stderr, "Failed to allocate output format chapters array.\n");
      ret = AVERROR(ENOMEM);
      return ret;
  }

  for (int i = 0; i < in_fmt_ctx->nb_chapters; i++)
  {
    in_chapter = in_fmt_ctx->chapters[i];
    if (!(out_chapter = av_mallocz(sizeof(AVChapter)))) {
      fprintf(stderr, "Failed to allocate out_chapter for chapter %d", i);
      ret = AVERROR(ENOMEM);
      return ret;
    }

    out_chapter->id = in_chapter->id;
    out_chapter->time_base = in_chapter->time_base;
    out_chapter->start = in_chapter->start;
    out_chapter->end = in_chapter->end;

    if ((ret = av_dict_copy(&out_chapter->metadata,
      in_chapter->metadata, 0)) < 0)
    {
      fprintf(stderr, "Failed to copy chapter metadata.\n");
      av_freep(&out_chapter);
      return ret;
    }
    out_fmt_ctx->chapters[i] = out_chapter;
    out_fmt_ctx->nb_chapters++;
  }
  return ret;
}

int initalize_stream(AVStream **stream, int *stream_idx, AVFormatContext *out_fmt_ctx,
  AVFormatContext *in_fmt_ctx, enum AVMediaType AVMEDIA_TYPE)
{
  int ret = 0;

  if ((ret = *stream_idx =
    av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE, -1, -1, NULL, 0)) < 0)
  {
    fprintf(stderr, "Failed to find video stream in input file.\n");
    return ret;
  }

  if (!(*stream = avformat_new_stream(out_fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate video output stream.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  if ((ret = avcodec_parameters_copy((*stream)->codecpar,
    in_fmt_ctx->streams[*stream_idx]->codecpar)) < 0)
  {
    fprintf(stderr, "failed to copy video codec parameters\n");
    return ret;
  }
  (*stream)->codecpar->codec_tag = 0;

  if ((ret = av_dict_copy(&(*stream)->metadata,
    in_fmt_ctx->streams[*stream_idx]->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy video metadata.\n");
    return ret;
  }
  return ret;
}

int main(int argc, char **argv)
{
  const char *v_filename, *s_filename, *out_filename, *title;
  AVFormatContext *v_fmt_ctx = NULL, *s_fmt_ctx = NULL, *out_fmt_ctx = NULL;
  const AVInputFormat *s_fmt;
  int v_stream_idx, a_stream_idx, s_stream_idx;
  AVStream *in_stream, *out_stream;
  AVPacket *pkt = NULL;
  int ret;

  if (argc != 5) {
    printf("\nUsage: %s <video file> <subtitle file> <output file> <title>\n\n\t"
      "This program performs all the same operations as cpmetadata.c. In addition,\n\t"
      "it also takes in an .srt file with subtitle information in it, creates a\n\t"
      "subtitle stream with the information, and embeds the stream in the output\n\t"
      "file. It is roughly the equivalent of the following ffmpeg command:\n\n\t"
      "ffmpeg -i  \"<video file>\" -i \"<subtitle file>\" -c copy \\\n\t\t"
      "-metadata:s:s:0 language=eng -metadata title= \"<title>\" \"<output file>\"\n"
      , argv[0]);
    return 0;
  }

  v_filename = argv[1];
  s_filename = argv[2];
  out_filename = argv[3];
  title = argv[4];

  if ((ret = avformat_open_input(&v_fmt_ctx, v_filename, NULL, NULL)) < 0) {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", v_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(v_fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  // av_dump_format(v_fmt_ctx, 0, v_filename, 0);

  if (!(s_fmt = av_find_input_format("webvtt"))) {
    fprintf(stderr, "Failed to find format for subtitle file.\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }

  if ((ret = avformat_open_input(&s_fmt_ctx, s_filename, s_fmt, NULL)) < 0) {
    fprintf(stderr, "Failed to open subtitle file: '%s'.\n", s_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(s_fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  printf("%s\n", s_fmt_ctx->iformat->name);

  if ((ret =
    avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, out_filename)))
  {
    fprintf(stderr, "Failed to allocate output format context.\n");
    goto end;
  }

  if ((ret = av_dict_copy(&out_fmt_ctx->metadata,
    v_fmt_ctx->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy file metadata.\n");
    goto end;
  }

  if ((ret = av_dict_set(&out_fmt_ctx->metadata, "title", title, 0)) < 0) {
    fprintf(stderr, "Failed to set title for output format context.\n");
    goto end;
  }

  if ((ret = copy_chapters(out_fmt_ctx, v_fmt_ctx)) < 0)
  {
    fprintf(stderr, "Failed to copy chapters.\n");
    goto end;
  }

  if ((ret = initalize_stream(&out_stream, &v_stream_idx, out_fmt_ctx, v_fmt_ctx,
    AVMEDIA_TYPE_VIDEO)) < 0)
  {
    fprintf(stderr, "Failed to initialize video stream.\n");
    goto end;
  }

  if ((ret = initalize_stream(&out_stream, &a_stream_idx, out_fmt_ctx, v_fmt_ctx,
    AVMEDIA_TYPE_AUDIO)) < 0)
  {
    fprintf(stderr, "Failed to initialize audio stream.\n");
    goto end;
  }

  // av_dump_format(out_fmt_ctx, 0, out_filename, 1);

  if ((ret = initalize_stream(&out_stream, &s_stream_idx, out_fmt_ctx, s_fmt_ctx,
    AVMEDIA_TYPE_SUBTITLE)) < 0)
  {
    fprintf(stderr, "Failed to initialize subtitle stream.\n");
    goto end;
  }

  if (strcmp(out_fmt_ctx->oformat->name, "mp4") == 0) {
    out_stream->codecpar->codec_id = AV_CODEC_ID_MOV_TEXT;
  }

  if ((ret = av_dict_set(&out_stream->metadata,
    "language", "eng", 0)) < 0)
  {
    fprintf(stderr, "Failed to set language for subtitles.\n");
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

  while ((ret = av_read_frame(v_fmt_ctx, pkt)) >= 0)
  {
    in_stream = v_fmt_ctx->streams[pkt->stream_index];

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

    out_stream = out_fmt_ctx->streams[pkt->stream_index];
    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
    pkt->pos = -1;

    if ((ret = av_interleaved_write_frame(out_fmt_ctx, pkt)) < 0) {
      fprintf(stderr, "Failed to write av packet to file.\n");
      goto end;
    }

    av_packet_unref(pkt);
  }

  while ((ret = av_read_frame(s_fmt_ctx, pkt)) >= 0)
  {
    in_stream = s_fmt_ctx->streams[pkt->stream_index];
    out_stream = out_fmt_ctx->streams[2];

    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
    pkt->stream_index = 2;
    pkt->pos = -1;

    if ((ret = av_interleaved_write_frame(out_fmt_ctx, pkt)) < 0) {
      fprintf(stderr, "Failed to write subtitle packet to file.\n");
      goto end;
    }

    av_packet_unref(pkt);
  }

  if ((ret = av_write_trailer(out_fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  avformat_close_input(&v_fmt_ctx);
  avformat_close_input(&s_fmt_ctx);

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
