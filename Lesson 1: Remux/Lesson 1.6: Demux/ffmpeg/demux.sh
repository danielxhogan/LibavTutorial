#!/bin/zsh
source ~/.zshrc

taskset -c 0-5,12-15 $HOME/ffmpeg/bin/ffmpeg \
  -i "./videos/inputs/all_ikkf.mkv" \
  -map 0:0 -c copy -f matroska -metadata title="all_ikkf.av1" "./videos/ffmpeg/all_ikkf.av1"\
  -map 0:1 -c copy -f matroska -metadata title="all_ikkf.opus" "./videos/ffmpeg/all_ikkf.opus"
