#!/bin/zsh
source ~/.zshrc

ffmpeg \
  -i "./videos/inputs/heretic.mkv" \
  -map 0:v -c:v libsvtav1 \
  -dolbyvision true \
  -strict unofficial \
  "./videos/ffmpeg/heretic.mp4"
