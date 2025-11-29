#pragma once

#include "input.h"
#include "output.h"

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include <stdio.h>

int avtranscode(const char *in_filename, const char *out_filename,
  char *video_codec, char *audio_codec, const char *map);
