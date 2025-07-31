#!/bin/zsh
source ~/.zshrc

ffmpeg \
  -i "../og/ll.mkv" \
  -map 0:v -c:v copy -map 0:a:5 -c:a aac \
  ./videos/inputs/ll_aac.mkv
