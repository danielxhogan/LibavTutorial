#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

typedef struct InputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *dec_ctx;
  AVFrame *dec_frame;
  int stream_idx;
} InputContext;

InputContext *open_input(const char *in_filename, int stream_idx)
{
  int ret = 0;
  AVStream *in_stream;
  const AVCodec *dec;

  InputContext *in_ctx = malloc(sizeof(InputContext));
  if (!in_ctx) {
    ret = ENOMEM;
    goto end;
  }

  in_ctx->fmt_ctx = NULL;
  in_ctx->dec_ctx = NULL;
  in_ctx->dec_frame = NULL;
  in_ctx->stream_idx = stream_idx;

  if ((ret = avformat_open_input(&in_ctx->fmt_ctx, in_filename, NULL, NULL)) < 0) {
    fprintf(stderr, "Failed to open input video file: '%s'.\n", in_filename);
    goto end;
  }

  if ((ret = avformat_find_stream_info(in_ctx->fmt_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream info.");
    goto end;
  }

  if (stream_idx >= in_ctx->fmt_ctx->nb_streams) {
    fprintf(stderr, "Invalid stream index for file '%s'\n", in_filename);
    ret = -1;
    goto end;
  }

  in_stream = in_ctx->fmt_ctx->streams[stream_idx];

  if (!(dec = avcodec_find_decoder(in_stream->codecpar->codec_id))) {
    fprintf(stderr, "Failed to find decoder.\n");
    ret = AVERROR(EINVAL);
    goto end;
  }

  if (!(in_ctx->dec_ctx = avcodec_alloc_context3(dec))) {
    fprintf(stderr, "Failed to allocate decoder context.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret =
    avcodec_parameters_to_context(in_ctx->dec_ctx, in_stream->codecpar)) < 0)
  {
    fprintf(stderr, "Failed to copy codec parameters to decoder context.\n");
    goto end;
  }

  if ((ret = avcodec_open2(in_ctx->dec_ctx, dec, NULL)) < 0) {
    fprintf(stderr, "Failed to open decoder.\n");
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
  avcodec_free_context(&in_ctx->dec_ctx);
  av_frame_free(&in_ctx->dec_frame);
  free(in_ctx);
}

typedef struct OutputContext {
  AVFormatContext *fmt_ctx;
  AVCodecContext *enc_ctx;
} OutputContext;

OutputContext *open_output(AVCodecContext *dec_ctx,
  const char *codec, const char *out_filename,
  AVDictionary *fmt_metadata, AVDictionary *stream_metadata)
{
  int ret;
  AVStream *out_stream;
  const AVCodec *enc;

  OutputContext *out_ctx = malloc(sizeof(OutputContext));
  if (!out_ctx) {
    ret = ENOMEM;
    goto end;
  }

  out_ctx->fmt_ctx = NULL;
  out_ctx->enc_ctx = NULL;

  if (!(enc = avcodec_find_encoder_by_name(codec))) {
    fprintf(stderr, "Failed to find encoder.\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }

  if (!(out_ctx->enc_ctx = avcodec_alloc_context3(enc))) {
    fprintf(stderr, "Failed to allocate encoder context.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = av_channel_layout_copy(&out_ctx->enc_ctx->ch_layout,
    &dec_ctx->ch_layout)) < 0)
  {
    fprintf(stderr, "Failed to set channel layout on encoder.\n");
    goto end;
  }

  out_ctx->enc_ctx->sample_fmt = dec_ctx->sample_fmt;
  out_ctx->enc_ctx->sample_rate = dec_ctx->sample_rate;
  out_ctx->enc_ctx->time_base = (AVRational) {1, out_ctx->enc_ctx->sample_rate};
  out_ctx->enc_ctx->bit_rate = dec_ctx->bit_rate;

  if ((ret = avcodec_open2(out_ctx->enc_ctx, enc, NULL)) < 0) {
    fprintf(stderr, "Failed to open encoder.\n");
    goto end;
  }

  if ((ret = avformat_alloc_output_context2(&out_ctx->fmt_ctx,
    NULL, NULL, out_filename)))
  {
    fprintf(stderr, "Failed to allocate output format context.\n");
    goto end;
  }

  if ((ret = av_dict_copy(&out_ctx->fmt_ctx->metadata,
    fmt_metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy file metadata.\n");
    goto end;
  }

  if (!(out_stream = avformat_new_stream(out_ctx->fmt_ctx, NULL))) {
    fprintf(stderr, "Failed to allocate new output stream.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  if ((ret = av_dict_copy(&out_stream->metadata,
    stream_metadata, AV_DICT_DONT_OVERWRITE)) < 0)
  {
    fprintf(stderr, "Failed to copy audio metadata.\n");
    goto end;
  }

  if ((ret = avcodec_parameters_from_context(out_stream->codecpar, out_ctx->enc_ctx))) {
    fprintf(stderr,
      "Failed to copy codec parameters from encoder context to stream.\n");
      goto end;
  }

  out_stream->time_base = out_ctx->enc_ctx->time_base;

  if (!(out_ctx->fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
    if ((ret = avio_open(&out_ctx->fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0)
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
  if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return NULL;
  }
  return out_ctx;
}

void close_output(OutputContext *out_ctx)
{
  if (!out_ctx) return;
  if (out_ctx->fmt_ctx && !(out_ctx->fmt_ctx->flags & AVFMT_NOFILE))
    avio_closep(&out_ctx->fmt_ctx->pb);
  avformat_free_context(out_ctx->fmt_ctx);
  avcodec_free_context(&out_ctx->enc_ctx);
  free(out_ctx);
}

int main(int argc, char **argv)
{
  const char *in_filename, *out_filename, *codec;
  int ret = 0;
  AVPacket *pkt = NULL;

  InputContext *input_ctx = NULL;
  OutputContext *output_ctx = NULL;

  if (argc != 4) {
    printf("\nUsage: %s <input file> <output file> <codec>\n\n\t"
      "This example will take in a file with an audio stream, \n\t"
      "transcode the video using the specified codec, and output \n\t"
      "the audio stream into a new file.\n\n", argv[0]);
    return 0;
  }

  in_filename = argv[1];
  out_filename = argv[2];
  codec = argv[3];

  if (!(input_ctx = open_input(in_filename, 1))) {
    fprintf(stderr, "Failed to open input.\n");
    goto end;
  }

  if (!(output_ctx = open_output(input_ctx->dec_ctx,
    codec, out_filename, input_ctx->fmt_ctx->metadata,
    input_ctx->fmt_ctx->streams[input_ctx->stream_idx]->metadata)))
  {
    fprintf(stderr, "Failed to open output.\n");
    goto end;
  }

  if (!(pkt = av_packet_alloc())) {
    fprintf(stderr, "Failed to allocate AVPacket.\n");
    ret = AVERROR(ENOMEM);
    goto end;
  }

  while ((ret = av_read_frame(input_ctx->fmt_ctx, pkt)) >= 0)
  {
    if (!(pkt->stream_index == input_ctx->stream_idx)) {
      av_packet_unref(pkt);
      continue;
    }

    if ((ret = avcodec_send_packet(input_ctx->dec_ctx, pkt)) < 0) {
      fprintf(stderr, "Failed to send packet to decoder.\n");
      goto end;
    }

    while ((ret = avcodec_receive_frame(input_ctx->dec_ctx,
      input_ctx->dec_frame)) >= 0)
    {
      if ((ret = avcodec_send_frame(output_ctx->enc_ctx,
        input_ctx->dec_frame)) < 0)
      {
        fprintf(stderr, "Failed to send frame to encoder.\n");
        goto end;
      }

      while ((ret = avcodec_receive_packet(output_ctx->enc_ctx, pkt)) >= 0)
      {
        pkt->stream_index = 0;

        if ((ret = av_interleaved_write_frame(output_ctx->fmt_ctx, pkt)) < 0) {
          fprintf(stderr, "Failed to write packet to file.\n");
          goto end;
        }
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
    fprintf(stderr, "Failed to read frame.\n");
    goto end;
  }

  if ((ret = avcodec_send_frame(output_ctx->enc_ctx, NULL)) < 0) {
    fprintf(stderr, "Failed to send frame to encoder.\n");
    goto end;
  }

  if ((ret = av_write_trailer(output_ctx->fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  close_input(input_ctx);
  close_output(output_ctx);
  av_packet_free(&pkt);

  if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return -1;
  }
  return 0;
}
