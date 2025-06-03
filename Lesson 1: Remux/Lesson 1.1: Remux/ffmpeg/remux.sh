#!/bin/zsh
source ~/.zshrc

# ffmpeg -i "./videos/inputs/h264_mp3.mp4" -c copy -metadata:s:a:0 title=dude "./videos/ffmpeg/h264_mp3.mp4"

# ffmpeg -i "./videos/inputs/vp9_opus.webm" -c copy -metadata:s:a:0 suh=dude "./videos/ffmpeg/vp9_opus.webm"

# ffmpeg -i "./videos/inputs/theora_vorbis.ogg" -c copy -metadata suh=dude "./videos/ffmpeg/theora_vorbis.ogg"