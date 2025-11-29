#include "output.h"

static int copy_chapters(AVFormatContext *out_fmt_ctx,
  AVFormatContext *in_fmt_ctx)
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

  for (unsigned int i = 0; i < in_fmt_ctx->nb_chapters; i++)
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

static int open_video_encoder(AVCodecContext **enc_ctx,
  AVStream *in_stream, const char *codec, char *in_filename)
{
  int ret;
  const AVCodec *enc;
  HdrMetadataContext *hdr_ctx;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder.\n");
    ret = AVERROR_UNKNOWN;
    return ret;
  }

  if (!(*enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder context.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  (*enc_ctx)->time_base = in_stream->time_base;
  (*enc_ctx)->framerate = in_stream->avg_frame_rate;

  (*enc_ctx)->width = in_stream->codecpar->width;
  (*enc_ctx)->height = in_stream->codecpar->height;
  (*enc_ctx)->pix_fmt = in_stream->codecpar->format;

  (*enc_ctx)->color_primaries = in_stream->codecpar->color_primaries;
  (*enc_ctx)->color_trc = in_stream->codecpar->color_trc;
  (*enc_ctx)->colorspace = in_stream->codecpar->color_space;
  (*enc_ctx)->color_range = in_stream->codecpar->color_range;
  (*enc_ctx)->chroma_sample_location = in_stream->codecpar->chroma_location;

  (*enc_ctx)->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  if (!(hdr_ctx = hdr_metadata_alloc())) {
    fprintf(stderr, "Failed to allocate hdr metada.\n");
    ret = -ENOMEM;
    return ret;
  }

  if ((ret = extract_hdr_metadata(hdr_ctx, in_filename)) < 0) {
    fprintf(stderr, "Failed to extract hdr metadata.\n");
    return ret;
  }

  if ((ret = inject_hdr_metadta(hdr_ctx, *enc_ctx)) < 0) {
    fprintf(stderr, "Failed to inject hdr metadata.\n");
    return ret;
  }

  if ((ret = avcodec_open2(*enc_ctx, enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    return ret;
  }

  return 0;
}

static int select_channel_layout(AVCodecContext *enc_ctx,
  AVChannelLayout *preferred_layout)
{
  const AVChannelLayout *layouts = NULL, *current_layout;
  char preferred_layout_name[64], current_layout_name[64];
  int preferred_nb_channels, ret = 0;

  if ((ret = avcodec_get_supported_config(enc_ctx, NULL,
    AV_CODEC_CONFIG_CHANNEL_LAYOUT, 0, (const void **) &layouts, NULL)) < 0)
  {
    fprintf(stderr, "Failed to get supported channel layouts.\n");
    return ret;
  }

  if ((ret = av_channel_layout_describe(preferred_layout,
    preferred_layout_name, sizeof(preferred_layout_name))) < 0)
  {
    fprintf(stderr, "Failed to get name for preferred layout.\n");
    return ret;
  }

  printf("preferred_layout: %s\n", preferred_layout_name);

  if (!layouts) {
    printf("No supported channel layouts list found. "
      "Attempting to set preferred layout.\n");

    if ((ret = av_channel_layout_copy(&enc_ctx->ch_layout,
      preferred_layout)) < 0)
    {
      fprintf(stderr, "Failed to copy preferred channel layout."
        "Attempting to set stereo channel layout\n");

      goto set_stereo;
    }
    
    printf("Channel layout set to %s.\n", preferred_layout_name);
    return 0;
  }

  printf("Checking if preferred layout is supported by encoder.\n");

  current_layout = layouts;

  while (current_layout->nb_channels) {
    if ((ret = av_channel_layout_describe(current_layout,
      current_layout_name, sizeof(current_layout_name))) < 0)
    {
      fprintf(stderr, "Failed to get name for current layout.\n");
      return ret;
    }

    printf("current_layout_name: %s\n", current_layout_name);

    if (!strcmp(current_layout_name, preferred_layout_name)) {
      if ((ret =
        av_channel_layout_copy(&enc_ctx->ch_layout, current_layout)) < 0)
      {
        fprintf(stderr, "Failed to copy preferred channel layout.\n");
        return ret;
      }

      printf("Channel layout set to preferred layout.\n");
      return 0;
    }

    current_layout++;
  }

  printf("Preferred layout not supported. Checking for supported layout "
    "with equivalent number of channels.\n");

  preferred_nb_channels = preferred_layout->nb_channels;
  current_layout = layouts;

  while (current_layout->nb_channels) {
    if ((ret = av_channel_layout_describe(current_layout, current_layout_name,
      sizeof(current_layout_name))) < 0)
    {
      fprintf(stderr, "Failed to get name of current layout.\n");
      return ret;
    }

    printf("current_layout_name: %s\n", current_layout_name);

    if (current_layout->nb_channels == preferred_nb_channels) {
      if ((ret =
        av_channel_layout_copy(&enc_ctx->ch_layout, current_layout)) < 0)
      {
        fprintf(stderr, "Failed to copy layout with preferred_nb_channels.\n");
        return ret;
      }

      printf("Channel layout set to %s.\n", current_layout_name);
      return 0;
    }

    current_layout++;
  }

  printf("No layout with equivalent number of channels supported. Attempting "
  "to set channel layout to stereo.\n");

set_stereo:
  if ((ret = av_channel_layout_copy(&enc_ctx->ch_layout,
    &(AVChannelLayout) AV_CHANNEL_LAYOUT_STEREO)) < 0)
  {
    fprintf(stderr, "Failed to copy stereo channel layout.\n");
    return ret;
  }

  printf("Channel layout set to stereo.\n");
  return 0;
}

int select_sample_fmt(AVCodecContext *enc_ctx,
  enum AVSampleFormat preferred_fmt)
{
  const enum AVSampleFormat *formats = NULL;
  const char *preferred_fmt_name, *current_fmt_name;
  int i = 0, ret = 0;

  if ((ret = avcodec_get_supported_config(enc_ctx, NULL,
    AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, (const void **) &formats, NULL)) < 0)
  {
    fprintf(stderr, "Failed to get supported sample formats.\n");
    return ret;
  }

  if (!(preferred_fmt_name = av_get_sample_fmt_name(preferred_fmt))) {
    fprintf(stderr, "Failed to get name of preferred_fmt.\n");
    return AVERROR_UNKNOWN;
  }

  printf("preferred format: %s\n", preferred_fmt_name);

  if (!formats) {
    printf("No supported sample formats list found. "
      "Setting sample format to preferred sample format.\n");

    enc_ctx->sample_fmt = preferred_fmt;
    return 0;
  }

  printf("Checking if preferred sample format is supported by encoder.\n");

  while (formats[i] && formats[i] != AV_SAMPLE_FMT_NONE)
  {
    if (!(current_fmt_name = av_get_sample_fmt_name(formats[i]))) {
      fprintf(stderr, "Failed to get name of current_fmt_name.\n");
      return AVERROR_UNKNOWN;
    }

    printf("current_fmt_name: %s\n", current_fmt_name);

    if (!strcmp(current_fmt_name, preferred_fmt_name))
    {
      enc_ctx->sample_fmt = formats[i];
      printf("Sample format set to preferred sample format.\n");
      return 0;
    }

    i++;
  }

  printf("Preferred sample format not supported. "
    "Setting sample format to first supported sample format.\n");

  if (!(current_fmt_name = av_get_sample_fmt_name(formats[0]))) {
    fprintf(stderr, "Failed to get name of first supported sample format.\n");
    return AVERROR_UNKNOWN;
  }

  printf("first supported sample format: %s\n", current_fmt_name);
  enc_ctx->sample_fmt = formats[0];

  return 0;
}

static int open_audio_encoder(AVCodecContext **enc_ctx,
  AVStream *in_stream, const char *codec)
{
  int ret;
  const AVCodec *enc;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder.\n");
    ret = AVERROR_UNKNOWN;
    return ret;
  }

  if (!(*enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder context.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  if ((ret = select_channel_layout(*enc_ctx,
    &in_stream->codecpar->ch_layout)) < 0)
  {
    fprintf(stderr, "Failed to select channel layout.\n");
    return ret;
  }

  if ((ret = select_sample_fmt(*enc_ctx,
    in_stream->codecpar->format)) < 0)
  {
    fprintf(stderr, "Failed to select sample format.\n");
    return ret;
  }

  (*enc_ctx)->sample_rate = in_stream->codecpar->sample_rate;
  (*enc_ctx)->bit_rate = in_stream->codecpar->bit_rate;

  if ((ret = avcodec_open2(*enc_ctx, enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    return ret;
  }

  return 0;
}

static int open_encoder(AVCodecContext **enc_ctx,
  AVStream *in_stream, char *vcodec, char *acodec, char *in_filename)
{
  int ret;
  enum AVMediaType stream_type = in_stream->codecpar->codec_type;

  if (stream_type == AVMEDIA_TYPE_VIDEO) {
    if ((ret =
      open_video_encoder(enc_ctx, in_stream, vcodec, in_filename)) < 0)
    {
      fprintf(stderr, "Failed to open video encoder for output stream.\n");
      return ret;
    }
  }
  else if (stream_type == AVMEDIA_TYPE_AUDIO) {
    if ((ret = open_audio_encoder(enc_ctx, in_stream, acodec)) < 0) {
      fprintf(stderr, "Failed to open audio encoder for output stream.\n");
      return ret;
    }
  }

  return 0;
}

static int init_stream(AVFormatContext *fmt_ctx,
  AVCodecContext *enc_ctx, AVStream *in_stream)
{
  int ret;
  AVStream *out_stream;

  if (!(out_stream = avformat_new_stream(fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate new output stream.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    in_stream->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy video metadata.\n");
    return ret;
  }

  if (enc_ctx) {
    if ((ret =
      avcodec_parameters_from_context(out_stream->codecpar, enc_ctx)) < 0)
    {
      fprintf(stderr,
        "Failed to copy codec parameters from encoder context to stream.\n");
      return ret;
    }

    out_stream->r_frame_rate = in_stream->r_frame_rate;
    out_stream->avg_frame_rate = in_stream->avg_frame_rate;
  }
  else {
    if ((ret =
      avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar)) < 0)
    {
      fprintf(stderr,
        "Failed to copy codec paramets from input stream to output stream.\n");
      return ret;
    }
  }

  return 0;
}

static int initialize_swr(struct SwrContext **swr_ctx, AVCodecContext *dec_ctx,
  AVCodecContext *enc_ctx, AVFrame **swr_frame)
{
  int ret = 0;

  if (!(*swr_ctx = swr_alloc())) {
    fprintf(stderr, "Failed to allocate SwrContext.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  av_opt_set_chlayout(*swr_ctx, "in_chlayout", &dec_ctx->ch_layout, 0);
  av_opt_set_int(*swr_ctx, "in_sample_rate", dec_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(*swr_ctx, "in_sample_fmt", dec_ctx->sample_fmt, 0);
  av_opt_set_chlayout(*swr_ctx, "out_chlayout", &enc_ctx->ch_layout, 0);
  av_opt_set_int(*swr_ctx, "out_sample_rate", enc_ctx->sample_rate, 0);
  av_opt_set_sample_fmt(*swr_ctx, "out_sample_fmt", enc_ctx->sample_fmt, 0);


  if ((ret = swr_init(*swr_ctx)) < 0) {
    fprintf(stderr, "Failed to initialize SwrContext.\n");
    return ret;
  }

  if (!(*swr_frame = av_frame_alloc())) {
    fprintf(stderr, "Failed to allocate AVFrame.\n");
    ret = AVERROR(ENOMEM);
    return ret;
  }

  (*swr_frame)->format = enc_ctx->sample_fmt;
  av_channel_layout_copy(&(*swr_frame)->ch_layout, &enc_ctx->ch_layout);
  (*swr_frame)->sample_rate = enc_ctx->sample_rate;
  (*swr_frame)->nb_samples = 1536;

  if ((ret = av_frame_get_buffer(*swr_frame, 0)) < 0) {
    fprintf(stderr, "Failed to allocate buffers for frame.\n");
    return ret;
  }

  return 0;
}

OutputContext *open_output(InputContext *in_ctx,
  const char *out_filename, char *vcodec, char *acodec)
{
  int ret, i, j;
  enum AVMediaType codec_type;

  OutputContext *out_ctx = malloc(sizeof(OutputContext));
  if (!out_ctx) {
    ret = -ENOMEM;
    goto end;
  }

  out_ctx->fmt_ctx = NULL;
  out_ctx->enc_ctx = NULL;
  out_ctx->swr_ctx = NULL;
  out_ctx->swr_frame = NULL;
  out_ctx->enc_pkt = NULL;
  out_ctx->fsc_ctx = NULL;
  out_ctx->nb_samples_encoded = NULL;
  out_ctx->nb_selected_streams = in_ctx->nb_selected_streams;

  if ((ret = avformat_alloc_output_context2(
    &out_ctx->fmt_ctx, NULL, NULL, out_filename)))
  {
    fprintf(stderr, "Failed to allocate output format context.\n");
    goto end;
  }

  if ((ret = av_dict_copy(&out_ctx->fmt_ctx->metadata,
    in_ctx->fmt_ctx->metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy file metadata.\n");
    goto end;
  }

  if ((ret = copy_chapters(out_ctx->fmt_ctx, in_ctx->fmt_ctx)) < 0)
  {
    fprintf(stderr, "Failed to copy chapters.\n");
    goto end;
  }

  if (!(out_ctx->enc_ctx =
    calloc(out_ctx->nb_selected_streams, sizeof(AVCodecContext *))))
  {
    fprintf(stderr, "Failed to allocate array for encoder contexts.\n");
    ret = -ENOMEM;
    goto end;
  }

  if (!(out_ctx->swr_ctx =
    calloc(out_ctx->nb_selected_streams, sizeof(SwrContext *))))
  {
    fprintf(stderr, "Failed to allocate array for swr contexts.\n");
    ret = -ENOMEM;
    goto end;
  }

  if (!(out_ctx->swr_frame =
    calloc(out_ctx->nb_selected_streams, sizeof(AVFrame *))))
  {
    fprintf(stderr, "Failed to allocate array for swr frames.\n");
    ret = -ENOMEM;
    goto end;
  }

  if (!(out_ctx->fsc_ctx =
    calloc(out_ctx->nb_selected_streams, sizeof(FrameSizeConversionContext *))))
  {
    fprintf(stderr, "Failed to allocate array for fsc contexts.\n");
    ret = -ENOMEM;
    goto end;
  }

  if (!(out_ctx->nb_samples_encoded =
    calloc(out_ctx->nb_selected_streams, sizeof(uint64_t))))
  {
    fprintf(stderr, "Failed to allocate array for nb_samples_encoded.\n");
    ret = -ENOMEM;
    goto end;
  }

  for (i = 0; i < out_ctx->nb_selected_streams; i++) {
    out_ctx->nb_samples_encoded[i] = 0;
  }

  j = 0;
  for (i = 0; i < out_ctx->nb_selected_streams; i++)
  {
    while (in_ctx->map[j] == INACTIVE_STREAM) {
      j++;
    }

    codec_type = in_ctx->fmt_ctx->streams[j]->codecpar->codec_type;

    if (codec_type == AVMEDIA_TYPE_VIDEO || codec_type == AVMEDIA_TYPE_AUDIO)
    {
      if ((ret = open_encoder(&out_ctx->enc_ctx[i],
        in_ctx->fmt_ctx->streams[j], vcodec, acodec, in_ctx->fmt_ctx->url)) < 0)
      {
        fprintf(stderr, "Failed to open encoder for output stream: %d\n", i);
        goto end;
      }

      if ((ret = init_stream(out_ctx->fmt_ctx, out_ctx->enc_ctx[i],
        in_ctx->fmt_ctx->streams[j])) < 0)
      {
        fprintf(stderr,
          "Failed to initialize stream for output stream: %d\n", i);
        goto end;
      }

      if (codec_type == AVMEDIA_TYPE_AUDIO)
      {
        if ((ret = initialize_swr(&out_ctx->swr_ctx[i], in_ctx->dec_ctx[j],
          out_ctx->enc_ctx[i], &out_ctx->swr_frame[i])))
        {
          fprintf(stderr,
            "Failed to initialize swr context for output stream: %d\n", i);
          goto end;
        }

        if (!(out_ctx->fsc_ctx[i] = fsc_ctx_alloc(out_ctx->enc_ctx[i]))) {
          fprintf(stderr,
            "Failed to allocate fsc context for output stream: %d\n", i);
          ret = -ENOMEM;
          goto end;
        }
      }
    } else {
      if ((ret = init_stream(out_ctx->fmt_ctx,
        NULL, in_ctx->fmt_ctx->streams[j])) < 0)
      {
        fprintf(stderr,
          "Failed to initialize stream for output stream: %d\n", i);
        goto end;
      }
    }

    j++;
  }

  if (!(out_ctx->enc_pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate output context packet.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if (!(out_ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if ((ret =
      avio_open(&out_ctx->fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0)
    {
      fprintf(stderr, "Failed to open output file.\n");
      goto end;
    }
  }

  if ((ret = avformat_write_header(out_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to write header for output file.\n");
    goto end;
  }

end:
  if (ret < 0) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return NULL;
  }
  return out_ctx;
}

void close_output(OutputContext *out_ctx)
{
  int i;

  if (!out_ctx) return;

  if (out_ctx->fmt_ctx && !(out_ctx->fmt_ctx->flags & AVFMT_NOFILE))
    avio_closep(&out_ctx->fmt_ctx->pb);
  avformat_free_context(out_ctx->fmt_ctx);

  if (out_ctx->enc_ctx) {
    for (i = 0; i < out_ctx->nb_selected_streams; i++) {
      avcodec_free_context(&out_ctx->enc_ctx[i]);
    }
    free(out_ctx->enc_ctx);
  }

  if (out_ctx->swr_ctx) {
    for (i = 0; i < out_ctx->nb_selected_streams; i++) {
      swr_free(&out_ctx->swr_ctx[i]);
    }
    free(out_ctx->swr_ctx);
  }

  if (out_ctx->swr_frame) {
    for (i = 0; i < out_ctx->nb_selected_streams; i++) {
      av_frame_free(&out_ctx->swr_frame[i]);
    }
    free(out_ctx->swr_frame);
  }

  if (out_ctx->fsc_ctx) {
    for (i = 0; i < out_ctx->nb_selected_streams; i++) {
      fsc_ctx_free(out_ctx->fsc_ctx[i]);
    }
    free(out_ctx->fsc_ctx);
  }

  av_packet_free(&out_ctx->enc_pkt);
  free(out_ctx->nb_samples_encoded);
  free(out_ctx);
}
