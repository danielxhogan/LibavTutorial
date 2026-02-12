# Back To The Future mpeg2
cd build && ninja && ./hwenc "../videos/inputs/mpeg2.mkv" "../videos/outputs/mpeg2.mkv" "cuda" "hevc_nvenc"
cd ..

# The Lost World vc1
cd build && ninja && ./hwenc "../videos/inputs/vc1.mkv" "../videos/outputs/vc1.mkv" "cuda" "hevc_nvenc"
cd ..

# GoldenEye h264
cd build && ninja && ./hwenc "../videos/inputs/h264.mkv" "../videos/outputs/h264.mkv" "cuda" "hevc_nvenc"
cd ..

# Furiosa hevc
cd build && ninja && ./hwenc "../videos/inputs/hevc.mkv" "../videos/outputs/hevc.mkv" "cuda" "hevc_nvenc"
cd ..

# cd build && ninja && ./hwenc
