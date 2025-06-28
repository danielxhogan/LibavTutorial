/*
 * This program takes in a video file and an output directory. It opens
 * the video file and loops through all streams. For each stream, it opens
 * an output file in the output directory specified and copies that stream
 * into the file along with any metadata for that stream, metadata for the
 * file, and any chapter information. Every file will be formatted using
 * the matroska format and the extension name for the file will be the codec
 * name of stream. The metadata title will be the same as the output filename.
 * It will be roughly the equivalent of the following ffmpeg command:
 * 
 * ffmpeg -i <input file> \
 *    -map 0:0 -c copy -f matroska \
 *    -metadata title="<input file basename>.<stream 0 codec>" \
 *    "<input file basename>.<stream 0 codec>" \
 *    ...
 *    -map 0:n -c copy -f matroska \
 *    -metadata title="<input file basename>.<stream n codec>" \
 *    "<input file basename>.<stream n codec>"
 */

#include <stdio.h>
#include <string.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>

typedef struct StreamContext {
  char *filename;
  AVFormatContext *fmt_ctx;
  AVStream *in_stream, *out_stream;
  int stream_idx;
} StreamContext;

int get_len_basename(size_t *len_basename, const char *filename)
{
  const char *slash;
  const char *dot;
  const char *end;
  size_t filename_length;
  size_t ext_length;

  slash = strrchr(filename, '/');
  if (slash) filename = ++slash;

  dot = strrchr(filename, '.');
  if (!dot || dot == filename) {
    fprintf(stderr, "Invalid file name.\n");
    return -1;
  }

  for (end = filename; *end; end++);
  filename_length = end - filename + 1;

  for (end = dot; *end; end++);
  ext_length = end - dot + 1;

  *len_basename = filename_length - ext_length;
  return 0;
}

int make_output_filename(StreamContext *stream_ctx, const char *output_dir,
  const char *basename, const char *ext)
{
  const char *end;
  size_t len_output_dir;
  size_t len_basename;
  size_t len_ext;
  size_t len_filename;

  for (end = output_dir; *end; end++);
  len_output_dir = end - output_dir + 1;

  for (end = basename; *end; end++);
  len_basename = end - basename + 1;

  for (end = ext; *end; end++);
  len_ext = end - ext + 1;

  len_filename = len_output_dir + len_basename + len_ext + 2;

  if (!(stream_ctx->filename = av_mallocz(len_filename + 1))) {
    printf("Could not allocate memory video output file name.\n");
    return AVERROR(ENOMEM);
  }

  strcat(stream_ctx->filename, output_dir);
  strcat(stream_ctx->filename, "/");
  strcat(stream_ctx->filename, basename);
  strcat(stream_ctx->filename, ".");
  strcat(stream_ctx->filename, ext);

  return 0;
}

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

int init_stream(StreamContext *stream_ctx, AVFormatContext *fmt_ctx,
  const char *output_dir, const char *basename)
{
  int ret;
  const char *ext, *title;

  stream_ctx->in_stream = fmt_ctx->streams[stream_ctx->stream_idx];
  ext = avcodec_get_name(stream_ctx->in_stream->codecpar->codec_id);

  if ((ret = make_output_filename(stream_ctx, output_dir,
    basename, ext)) != 0)
  {
    fprintf(stderr, "Failed to generate output filename.\n");
    return ret;
  }

  if ((ret = avformat_alloc_output_context2(&stream_ctx->fmt_ctx, NULL,
    "matroska", stream_ctx->filename)))
  {
    fprintf(stderr, "Failed to allocate format context for stream %d\n",
      stream_ctx->stream_idx);
    return ret;
  }

  if ((ret = av_dict_copy(&stream_ctx->fmt_ctx->metadata,
    fmt_ctx->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy file metadata.\n");
    return ret;
  }

  title = strrchr(stream_ctx->filename, '/');
  if (title) title = ++title;

  if ((ret = av_dict_set(&stream_ctx->fmt_ctx->metadata,
    "title", title, 0)) < 0)
  {
    fprintf(stderr, "Failed to set title for output format context.\n");
    return ret;
  }

  if ((ret = copy_chapters(stream_ctx->fmt_ctx, fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to copy chapters.\n");
    return ret;
  }

  if (!(stream_ctx->out_stream =
    avformat_new_stream(stream_ctx->fmt_ctx, NULL)))
  {
    fprintf(stderr, "Failed to allocate new output stream for stream %d\n",
      stream_ctx->stream_idx);
    ret = AVERROR(ENOMEM);
    return ret;
  }

  if ((ret = avcodec_parameters_copy(stream_ctx->out_stream->codecpar,
    stream_ctx->in_stream->codecpar)) < 0)
  {
    fprintf(stderr, "Failed to copy codec params for stream %d\n",
      stream_ctx->stream_idx);
    return ret;
  }

  if ((ret = av_dict_copy(&stream_ctx->out_stream->metadata,
    stream_ctx->in_stream->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy stream metadata.\n");
    return ret;
  }

  if (!(stream_ctx->fmt_ctx->flags & AVFMT_NOFILE)) {
    if ((ret = avio_open(&stream_ctx->fmt_ctx->pb,
      stream_ctx->filename, AVIO_FLAG_WRITE)) < 0)
    {
      fprintf(stderr, "Failed to open file for stream %d\n",
        stream_ctx->stream_idx);
      return ret;
    }
  }

  if ((ret = avformat_write_header(stream_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Error writing header to file for stream %d\n",
      stream_ctx->stream_idx);
    return ret;
  }

  return 0;
}

int main(int argc, char **argv)
{
  const char *in_filename, *slash, *output_dir;
  size_t len_basename = -1;
  char *basename = NULL;
  AVPacket *pkt = NULL;
  AVFormatContext *fmt_ctx = NULL;
  StreamContext **streams = NULL;
  int ret, i, stream_idx;

  if (argc != 3) {
    printf("\nusage: %s <input file> <output directory>\n\n\t"
      "This program takes in a video file and an output directory. It opens\n\t"
      "the video file and loops through all streams. For each stream, it\n\t"
      "opens an output file in the output directory specified and copies\n\t"
      "that stream into the file along with any metadata for that stream,\n\t"
      "metadata for the file, and any chapter information. Every file will\n\t"
      "be formatted using the matroska format and the extension name for\n\t"
      "the file will be the codec name of stream. The metadata title will\n\t"
      "be the same as the output filename. It will be roughly the\n\t"
      "equivalent of the following ffmpeg command:\n\n\t"
      "ffmpeg -i <input file> \\\n\t\t"
      "   -map 0:0 -c copy -f matroska \\\n\t\t"
      "   -metadata title=\"<input file basename>.<stream 0 codec>\" \\\n\t\t"
      "   \"<input file basename>.<stream 0 codec>\" \\\n\t\t"
      "   ...\n\t\t"
      "   -map 0:n -c copy -f matroska \\\n\t\t"
      "   -metadata title=\"<input file basename>.<stream n codec>\" \\\n\t\t"
      "   \"<input file basename>.<stream n codec>\"\\n\t\t"
      "\n", argv[0]);
    return 1;
  }

  in_filename = argv[1];
  output_dir = argv[2];

  if ((ret = avformat_open_input(&fmt_ctx, in_filename, 0, 0)) < 0) {
    fprintf(stderr, "Failed to open input file '%s'.\n", in_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(fmt_ctx, 0)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  slash = strrchr(in_filename, '/');
  if (slash) in_filename = ++slash;

  if ((ret = get_len_basename(&len_basename, in_filename)) < 0) {
    printf("Failed to get length of input file basename.\n");
    goto end;
  }

  if (!(basename = av_mallocz((len_basename + 1) * sizeof(char)))) {
    printf("Failed to allocate memory for basename.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }
  strncpy(basename, in_filename, len_basename);

  if (!(streams = av_calloc(fmt_ctx->nb_streams, sizeof(StreamContext *)))) {
    fprintf(stderr, "Could not allocate memory for streams array.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  // av_dump_format(fmt_ctx, 0, in_filename, 0);

  for (i = 0; i < fmt_ctx->nb_streams; i++)
  {
    if (!(streams[i] = av_mallocz(sizeof(StreamContext)))) {
      fprintf(stderr, "Could not allocate memory "
        "for streams for stream '%d'.\n", i);
      ret = AVERROR(ENOMEM);
      goto end;
    }

    streams[i]->filename = NULL;
    streams[i]->stream_idx = i;
    if ((ret = init_stream(streams[i], fmt_ctx, output_dir, basename)) < 0) {
      goto end;
    }
  }

  if (!(pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    goto end;
  }

  while ((ret = av_read_frame(fmt_ctx, pkt)) >= 0)
  {
    stream_idx = pkt->stream_index;
    pkt->stream_index = 0;
    av_packet_rescale_ts(pkt, streams[stream_idx]->in_stream->time_base,
      streams[stream_idx]->out_stream->time_base);

    if ((ret =
      av_interleaved_write_frame(streams[stream_idx]->fmt_ctx, pkt)) < 0)
    {
      fprintf(stderr, "Error writing packet to file for stream %d\n",
        stream_idx);
      goto end;
    }
    av_packet_unref(pkt);
  }

  for (i = 0; i < fmt_ctx->nb_streams; i++)
  {
    if ((ret = av_write_trailer(streams[i]->fmt_ctx)) < 0) {
      fprintf(stderr, "Error writing trailer for stream %d\n",
        streams[i]->stream_idx);
      goto end;
    }
  }

end:
  av_packet_free(&pkt);
  av_freep(&basename);

  if (streams) {
    for (i = 0; i < fmt_ctx->nb_streams; i++) {
      if (streams[i])
      {
        av_freep(&streams[i]->filename);
        if (streams[i]->fmt_ctx && !(streams[i]->fmt_ctx->flags & AVFMT_NOFILE))
          avio_closep(&streams[i]->fmt_ctx->pb);
        avformat_free_context(streams[i]->fmt_ctx);
        av_freep(&streams[i]);
      }
    }
    av_freep(&streams);
  }

  avformat_close_input(&fmt_ctx);

  if (ret < 0 && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return -1;
  }
  return 0;
}
