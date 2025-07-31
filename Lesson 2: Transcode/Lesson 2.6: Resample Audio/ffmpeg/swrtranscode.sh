#!/bin/zsh
source ~/.zshrc

# ffmpeg \
#   -i "./videos/inputs/ll_dts_51.mkv" \
#   -map 0:a -c:a libfdk_aac -b:a 1536k \
#   "./videos/ffmpeg/ll_dts_to_aac_51.mkv"

ffmpeg \
  -i "./videos/inputs/noes_ac3_51.mkv" \
  -map 0:a -c:a libfdk_aac -b:a 448k \
  "./videos/ffmpeg/noes_ac3_to_aac_51.mkv"
