#pragma once

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/dovi_meta.h>

typedef struct HdrMetadataContext {
  AVMasteringDisplayMetadata *mdm;
  AVContentLightMetadata *cll;
  AVDOVIMetadata *dovi;
} HdrMetadataContext;

HdrMetadataContext *hdr_metadata_alloc();

int extract_hdr_metadata(HdrMetadataContext *hdr_ctx, const char *filename);
int inject_hdr_metadta(HdrMetadataContext *hdr_ctx, AVCodecContext *enc_ctx);
