# # Back To The Future mpeg2
# cd build && ninja && ./hwdec "../videos/inputs/mpeg2.mkv" "../videos/outputs/mpeg2_x265_params.mkv" "cuda" "libx265" "crf=20"
# cd ..

# # The Lost World vc1
# cd build && ninja && ./hwdec "../videos/inputs/vc1.mkv" "../videos/outputs/vc1_x265_params.mkv" "cuda" "libx265" "crf=20"
# cd ..

# GoldenEye h264
# cd build && ninja && ./hwdec "../videos/inputs/h264.mkv" "../videos/outputs/h264_x265_params.mkv" "cuda" "libx265" "crf=20"
# cd ..

# # Furiosa hevc
# cd build && ninja && ./hwdec "../videos/inputs/hevc.mkv" "../videos/outputs/hevc_x265_params.mkv" "cuda" "libx265" "crf=20"
# cd ..

cd build && ninja && ./hwdec
