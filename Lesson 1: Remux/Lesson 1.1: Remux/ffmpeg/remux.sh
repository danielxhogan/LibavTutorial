#!/bin/zsh
source ~/.zshrc

ffmpeg -i "./videos/inputs/vp9_opus.webm" -c copy "./videos/outputs/vp9_opus.flv"