#!/bin/zsh
source ~/.zshrc

ffmpeg -i "./videos/inputs/h264_eac3.mkv" -c copy -metadata title="h264_eac3.mp4" "./videos/ffmpeg/h264_eac3.mp4"
