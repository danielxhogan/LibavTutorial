/*
 * This program creates an output file that copies streams from one or more
 * input files. The program expects a map for each input file that determines
 * the streams to be included in the output. Any stream not specified is
 * ignored. A stream can only be chosen once. The streams will be inserted into
 * the output in the order they appear in the command. It can handle up to 10
 * streams per input file. The metadata and chapter information for the output
 * file will be taken from the first input. The title will be set by the value
 * passed in <title> parameter. It will be roughly the equivalent of the
 * following ffmpeg command:
 *
 * ffmpeg \
 *   -i "<input file 1>" \
 *    ...
 *   -i "<input file n" \
 *   -c copy \
 *   -map <map 1> \
 *    ...
 *   -map <map n> \
 *   -metadata title="<title>" "<output file>"
 * 
 */

#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>

#define INACTIVE_STREAM -1

typedef struct InputContext {
  AVFormatContext *fmt_ctx;
  const char *filename;
  int *map;
} InputContext;


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

int initialize_stream(AVFormatContext *out_fmt_ctx,
  AVStream *in_stream, int input_idx, int stream_idx)
{
  AVStream *out_stream;
  int ret = 0;

  if (!(out_stream = avformat_new_stream(out_fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate output stream for "
      "input '%d' stream '%d'.\n", input_idx, stream_idx);
    ret = AVERROR(ENOMEM);
    return ret;
  }

  if ((ret = avcodec_parameters_copy(out_stream->codecpar,
    in_stream->codecpar)) < 0)
  {
    fprintf(stderr, "Failed to copy codec parameters "
      "for input '%d' stream'%d'.\n", input_idx, stream_idx);
    return ret;
  }
  out_stream->codecpar->codec_tag = 0;

  if ((out_stream->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE) &&
    !(strcmp(out_fmt_ctx->oformat->name, "mp4")))
  {
    out_stream->codecpar->codec_id = AV_CODEC_ID_MOV_TEXT;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    in_stream->metadata,
    AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy metadata for input '%d' stream '%d'.\n",
      input_idx, stream_idx);
    return ret;
  }
  return ret;
}

int initialize_input(InputContext *input_ctx,
  int *out_stream_idx, AVFormatContext *out_fmt_ctx,
  int input_idx, char **argv)
{
  const char *map;
  int ret, i, in_stream_idx;
  AVStream *in_stream;

  map = argv[(input_idx * 4) + 4];
  input_ctx->filename = argv[(input_idx * 4) + 2];

  if ((ret = avformat_open_input(&input_ctx->fmt_ctx,
    input_ctx->filename, NULL, NULL)) < 0)
  {
    fprintf(stderr, "Failed to open input file: '%s'.\n", input_ctx->filename);
    return ret;
  }

  if ((ret = avformat_find_stream_info(input_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info file: '%s'.\n",
      input_ctx->filename);
    return ret;
  }

  if (!(input_ctx->map =
    av_calloc(input_ctx->fmt_ctx->nb_streams, sizeof(int))))
  {
    fprintf(stderr, "Failed to allocate map array for input '%d'.\n",
      input_idx);
    ret = AVERROR(ENOMEM);
    return ret;
  }

  for (i = 0; i < input_ctx->fmt_ctx->nb_streams; i++) {
    input_ctx->map[i] = INACTIVE_STREAM;
  }

  for (i = 0; map[i] != '\0'; i++)
  {
    in_stream_idx = map[i] - '0';
    if (in_stream_idx < 0 || in_stream_idx > 9) {
      fprintf(stderr, "Invalid character found "
        "when parsing map for input '%d'.\n", input_idx);
      return 0;
    }

    if (in_stream_idx >= input_ctx->fmt_ctx->nb_streams) {
      fprintf(stderr, "Stream index '%d' does not exist for input '%d'.\n",
        in_stream_idx, input_idx);
      return 0;
    }

    in_stream = input_ctx->fmt_ctx->streams[in_stream_idx];
    input_ctx->map[in_stream_idx] = *out_stream_idx;
    *out_stream_idx += 1;

    if ((ret =
      initialize_stream(out_fmt_ctx, in_stream, input_idx, in_stream_idx)) < 0)
    {
      fprintf(stderr, "Failed to initialize stream '%d' for input '%d'.\n",
        i, input_idx);
      return ret;
    }
  }

  return ret;
}

int main(int argc, char **argv)
{
  const char *out_filename, *title;
  int nb_inputs = 0;
  InputContext **inputs = NULL;
  AVFormatContext *out_fmt_ctx = NULL;
  AVStream *in_stream, *out_stream;
  AVPacket *pkt = NULL;
  int ret = 0, out_stream_idx = 0, i;

  if (argc < 7) {
    printf("\nUsage: %s -i <input file 1> -map <map 1, eg. 012> \\\n\t\t"
      "[... -i <input file n> -map <map n>] <output file> <title>\n\n\t"
      "This program creates an output file that copies streams from one or\n\t"
      "more input files. The program expects a map for each input file that\n\t"
      "determines the streams to be included in the output. Any stream not\n\t"
      "specified is ignored. A stream can only be chosen once. The streams\n\t"
      "will be inserted into the output in the order they appear in the\n\t"
      "command. It can handle up to 10 streams per input file. The metadata\n\t"
      "and chapter information for the output file will be taken from the\n\t"
      "first input. The title will be set by the value passed in the\n\t"
      "<title> parameter. It will be roughly the equivalent of the\n\t"
      "following ffmpeg command:\n\n\t"
      "ffmpeg -i \"<input file 1>\" -map <map 1> -c copy \\\n\t\t"
      "[... -i \"<input file n>\" -map <map n>] \\\n\t\t"
      "-metadata title=\"<title>\" \"<output file>\"\n\n"
      , argv[0]);
    return 0;
  }

  out_filename = argv[argc - 2];
  title = argv[argc - 1];

  for (i = 1; i < argc - 3; i++) {
    if (!(strcmp(argv[i], "-i"))) nb_inputs++;
    if (!(strcmp(argv[i], "-map"))) nb_inputs--;
  }

  if (nb_inputs != 0) {
    fprintf(stderr, "Number of inputs must match number of maps.\n");
    goto end;
  }
  nb_inputs = (i + 1) / 4;

  if (!(inputs = av_calloc(nb_inputs, sizeof(InputContext *)))) {
    fprintf(stderr, "Failed to allocate InputContext array.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  for (i = 0; i < nb_inputs; i++) {
    if (!(inputs[i] = av_mallocz(sizeof(InputContext)))) {
      fprintf(stderr, "Failed to allocate Input Context for input '%d'.\n", i);
      ret = AVERROR(ENOMEM);
      goto end;
    }
  }

  if ((ret =
    avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, out_filename)))
  {
    fprintf(stderr, "Failed to allocate output format context.\n");
    goto end;
  }


  for (i = 0; i < nb_inputs; i++)
  {
    if ((ret = initialize_input(inputs[i],
      &out_stream_idx, out_fmt_ctx, i, argv)) < 0)
    {
      fprintf(stderr, "Failed to initialize input '%d'.\n", i);
      goto end;
    }
  }

  if ((ret = av_dict_copy(&out_fmt_ctx->metadata,
    inputs[0]->fmt_ctx->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy file metadata.\n");
    goto end;
  }

  if ((ret = av_dict_set(&out_fmt_ctx->metadata, "title", title, 0)) < 0) {
    fprintf(stderr, "Failed to set title for output format context.\n");
    goto end;
  }

  if ((ret = copy_chapters(out_fmt_ctx, inputs[0]->fmt_ctx)) < 0)
  {
    fprintf(stderr, "Failed to copy chapters.\n");
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

  for (i = 0; i < nb_inputs; i++) {
    while ((ret = av_read_frame(inputs[i]->fmt_ctx, pkt)) >= 0)
    {
      in_stream = inputs[i]->fmt_ctx->streams[pkt->stream_index];

      pkt->stream_index = inputs[i]->map[pkt->stream_index];

      if (pkt->stream_index == INACTIVE_STREAM) {
        av_packet_unref(pkt);
        continue;
      }

      out_stream = out_fmt_ctx->streams[pkt->stream_index];
      av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
      pkt->pos = -1;

      if ((ret = av_interleaved_write_frame(out_fmt_ctx, pkt)) < 0) {
        fprintf(stderr, "Failed to write packet to file "
          "for input '%d' stream '%d'.\n", i, in_stream->index);
        return ret;
      }
    }
  }

  if ((ret = av_write_trailer(out_fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  if (inputs) {
    for (i = 0; i < nb_inputs; i++) {
      avformat_close_input(&inputs[i]->fmt_ctx);
      av_freep(&inputs[i]->map);
      av_freep(&inputs[i]);
    }
    av_freep(&inputs);
  }

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
