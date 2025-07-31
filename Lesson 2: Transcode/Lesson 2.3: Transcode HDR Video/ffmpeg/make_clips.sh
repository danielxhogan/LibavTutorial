#!/bin/zsh
source ~/.zshrc

ffmpeg \
  -ss 23:42 \
  -i "/media/hugexjackedman/X10 Pro/4k_encoding/Heretic/rip/Heretic.mkv" \
  -t 10 \
  -map 0 -c copy \
  ./videos/inputs/her_dovi.mkv

# ffmpeg \
#   -ss 23:40 \
#   -i "/media/hugexjackedman/X10 Pro/4k_encoding/Heretic/rip/Heretic.mkv" \
#   -t 10:00 \
#   -map 0 -c copy \
#   ./videos/inputs/heretic_10min.mkv
