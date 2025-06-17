cd build && ninja && ./streams \
  -i "../videos/inputs/av1_mp3.mp4" -map 01 \
  -i "../videos/inputs/h264_eac3.mkv" -map 012 \
  -i "../videos/inputs/vp9_opus.webm" -map 01 \
  "../videos/outputs/all.mkv" "all.mkv"

# cd build && ninja && ./streams \
#   -i "../videos/outputs/all.mkv" -map 025 \
#   "../videos/outputs/video.mkv" "video.mkv"

# cd build && ninja && ./streams \
#   -i "../videos/inputs/av1_mp3.mp4" -map 0 \
#   -i "../videos/inputs/h264_eac3.mkv" -map 1 \
#   -i "../videos/inputs/vp9_opus.webm" -map 2 \
#   "../videos/outputs/av1_eac3.mkv" "av1_eac3.mkv"

# cd build && ninja && ./streams -i "../videos/inputs/ao_ikkf.mkv" -map 012 \
#   -i "../videos/inputs/vf_ikkf.mp4" -map 01 \
#   -i "../videos/inputs/h4m_ikkf.flv" -map 01 \
#   -i "../videos/inputs/tv_ikkf.ogg" -map 01 \
#   -i "../videos/inputs/h5a_ikkf.mov" -map 01 \
#   "../videos/outputs/all_ikkf.mkv" "all_ikkf.mkv"

# cd build && ninja && ./streams
