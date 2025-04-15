#!/bin/zsh
source ~/.zshrc

ffmpeg -i "./videos/og_ikkf.mkv" -c copy "./videos/ff_ikkf.mp4"
