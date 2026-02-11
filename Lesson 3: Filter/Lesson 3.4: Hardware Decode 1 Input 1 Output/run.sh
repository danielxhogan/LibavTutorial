# # Back To The Future mpeg2
# cd build && ninja && ./hwdec1in1out "../videos/inputs/mpeg2.mkv" "../videos/outputs/mpeg2.mkv" \
#   "libplacebo=format=yuv420p10le:color_primaries=bt2020:color_trc=smpte2084:colorspace=bt2020nc:chroma_location=left:range=tv,yadif" "cuda" "libx265" "crf=20"
# cd ..

# # The Lost World vc1
# cd build && ninja && ./hwdec1in1out "../videos/inputs/vc1.mkv" "../videos/outputs/vc1.mkv" "format=yuv420p10le,zscale=p=bt709:t=bt709:m=bt709:c=left" "cuda" "libx265" "crf=20"
# cd ..

# GoldenEye h264
# cd build && ninja && ./hwdec1in1out "../videos/inputs/h264.mkv" "../videos/outputs/h264.mkv" "format=yuv420p10le,zscale=p=bt709:t=bt709:m=bt709:c=left" "cuda" "libx265" "crf=20"
# cd ..

# Furiosa hevc
cd build && ninja && ./hwdec1in1out "../videos/inputs/hevc.mkv" "../videos/outputs/hevc.mkv" \
  "libplacebo=format=p010le:tonemapping=hable:color_primaries=bt709:color_trc=bt709:colorspace=bt709:chroma_location=left" "cuda" "libx265" "crf=20"
cd ..

# cd build && ninja && ./hwdec1in1out
