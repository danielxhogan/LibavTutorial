#!/bin/zsh
source ~/.zshrc

ffmpeg \
  -i "./videos/og_ikkf.mkv" \
  -i "./videos/tc_ikkf.mkv" \
  -c copy \
  -map 0:0 -map 0:1 \
  -map 1:1 -map 1:0 -map 1:2 \
  -metadata title="ff_ikkf.mkv" "./videos/ff_ikkf.mkv"
