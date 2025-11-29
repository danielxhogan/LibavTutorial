#include "avtranscode.h"

static int encode_video_frame(OutputContext *out_ctx, int out_stream_idx,
  InputContext *in_ctx, int in_stream_idx)
{
  int ret;

  if ((ret = avcodec_send_frame(
    out_ctx->enc_ctx[out_stream_idx], in_ctx->dec_frame)) < 0)
  {
    fprintf(stderr, "Failed to send frame to encoder.\n");
    return ret;
  }

  while ((ret = avcodec_receive_packet(
    out_ctx->enc_ctx[out_stream_idx], out_ctx->enc_pkt)) >= 0)
  {
    out_ctx->enc_pkt->stream_index = out_stream_idx;

    av_packet_rescale_ts(out_ctx->enc_pkt,
      in_ctx->fmt_ctx->streams[in_stream_idx]->time_base,
      out_ctx->fmt_ctx->streams[out_stream_idx]->time_base);

    if ((ret = av_interleaved_write_frame(out_ctx->fmt_ctx, out_ctx->enc_pkt)) < 0) {
      fprintf(stderr, "Failed to write packet to file.\n");
      return ret;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to receive packet from encoder.\n");
    return ret;
  }

  return 0;
}

static int encode_audio_frame(OutputContext *out_ctx, int out_stream_idx,
  InputContext *in_ctx)
{
  int ret, nb_converted_samples;

  AVFrame *swr_frame = out_ctx->swr_frame[out_stream_idx];
  FrameSizeConversionContext *fsc_ctx = out_ctx->fsc_ctx[out_stream_idx];

  if ((ret = av_frame_make_writable(swr_frame)) < 0) {
    fprintf(stderr, "Failed to make frame writable.\n");
    return ret;
  }

  if ((ret = nb_converted_samples =
    swr_convert(out_ctx->swr_ctx[out_stream_idx], swr_frame->data,
      swr_frame->nb_samples, (const uint8_t **) in_ctx->dec_frame->data,
      in_ctx->dec_frame->nb_samples)) < 0)
  {
    fprintf(stderr, "Failed to convert audio frame.\n");
    return ret;
  }

  if ((ret = fsc_ctx_add_samples_to_buffer(
    fsc_ctx, swr_frame, nb_converted_samples)) < 0)
  {
    fprintf(stderr, "Failed to add samples to buffer.\n");
    return ret;
  }

  while (fsc_ctx->nb_samples_in_buffer >=
    out_ctx->enc_ctx[out_stream_idx]->frame_size)
  {
    if ((ret = fsc_ctx_make_frame(fsc_ctx,
      out_ctx->nb_samples_encoded[out_stream_idx])) < 0)
    {
      fprintf(stderr, "Failed to make frame for encoder.\n");
      return ret;
    }

    out_ctx->nb_samples_encoded[out_stream_idx] +=
      out_ctx->enc_ctx[out_stream_idx]->frame_size;

    if ((ret =
      avcodec_send_frame(out_ctx->enc_ctx[out_stream_idx], fsc_ctx->frame)) < 0)
    {
      fprintf(stderr, "Failed to send frame to encoder.\n");
      return ret;
    }

    while ((ret = avcodec_receive_packet(
      out_ctx->enc_ctx[out_stream_idx], out_ctx->enc_pkt)) >= 0)
    {
      out_ctx->enc_pkt->stream_index = out_stream_idx;

      av_packet_rescale_ts(out_ctx->enc_pkt,
        (AVRational) {1, out_ctx->enc_ctx[out_stream_idx]->sample_rate},
        out_ctx->fmt_ctx->streams[out_stream_idx]->time_base);

      if ((ret =
        av_interleaved_write_frame(out_ctx->fmt_ctx, out_ctx->enc_pkt)) < 0)
      {
        fprintf(stderr, "Failed to write packet to file.\n");
        return ret;
      }
    }

    if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
      fprintf(stderr, "Failed to receive packet from encoder.\n");
      return ret;
    }
  }

  return 0;
}

int avtranscode(const char *in_filename, const char *out_filename,
  char *video_codec, char *audio_codec, const char *selected_streams)
{
  int ret = 0;

  InputContext *in_ctx = NULL;
  OutputContext *out_ctx = NULL;
  enum AVMediaType codec_type;
  int in_stream_idx, out_stream_idx;

  if (!(in_ctx = open_input(in_filename, selected_streams))) {
    fprintf(stderr, "Failed to open input.\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }

  if (!(out_ctx = open_output(in_ctx, out_filename, video_codec, audio_codec))) {
    fprintf(stderr, "Failed to open output.\n");
    ret = AVERROR_UNKNOWN;
    goto end;
  }

  while ((ret = av_read_frame(in_ctx->fmt_ctx, in_ctx->pkt)) >= 0)
  {
    in_stream_idx = in_ctx->pkt->stream_index;
    out_stream_idx = in_ctx->map[in_stream_idx];

    if (out_stream_idx == INACTIVE_STREAM) {
      av_packet_unref(in_ctx->pkt);
      continue;
    }

    if (!in_ctx->dec_ctx[in_stream_idx]) {

      in_ctx->pkt->stream_index = out_stream_idx;

      if ((ret =
        av_interleaved_write_frame(out_ctx->fmt_ctx, in_ctx->pkt)) < 0)
      {
        fprintf(stderr, "Failed to write packet to file.\n");
        goto end;
      }

      continue;
    }

    if ((ret =
      avcodec_send_packet(in_ctx->dec_ctx[in_stream_idx],
        in_ctx->pkt)) < 0)
    {
      fprintf(stderr,
        "Failed to send packet from input stream: %d to decoder.\n",
        in_stream_idx);
      goto end;
    }

    while ((ret =
      avcodec_receive_frame(in_ctx->dec_ctx[in_stream_idx],
        in_ctx->dec_frame)) >= 0)
    {
      codec_type = in_ctx->dec_ctx[in_stream_idx]->codec_type;

      if (codec_type == AVMEDIA_TYPE_VIDEO)
      {
        if ((ret = encode_video_frame(out_ctx, out_stream_idx,
          in_ctx, in_stream_idx)) < 0)
        {
          fprintf(stderr,
            "Failed to encode video frame from input stream: %d.\n",
            in_stream_idx);
        }
      }
      else if (codec_type == AVMEDIA_TYPE_AUDIO)
      {
        if ((ret = encode_audio_frame(out_ctx, out_stream_idx, in_ctx)) < 0) {
          fprintf(stderr,
            "Failed to encode audio frame from input stream: %d.\n",
            in_stream_idx);
        }
      }
    }

    if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
      fprintf(stderr,
        "Failed to receive frame from input stream: %d from decoder.\n",
        in_ctx->pkt->stream_index);
      goto end;
    }
  }

  if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF)) {
    fprintf(stderr, "Failed to read frame.\n");
    goto end;
  }

  for (int i = 0; i < out_ctx->nb_selected_streams; i++) {
    if (out_ctx->enc_ctx[i]) {
      if ((ret = avcodec_send_frame(out_ctx->enc_ctx[i], NULL)) < 0) {
        fprintf(stderr,
          "Failed to send null frame to encoder for output stream: %d.\n", i);
        goto end;
      }
    }
  }

  if ((ret = av_write_trailer(out_ctx->fmt_ctx)) < 0) {
    fprintf(stderr, "Failed to write trailer to file.\n");
    goto end;
  }

end:
  close_input(in_ctx);
  close_output(out_ctx);

  if (ret < 0 && ret != AVERROR_EOF) {
    fprintf(stderr, "\nLibav Error: %s\n", av_err2str(ret));
    return -1;
  }
  return 0;
}
