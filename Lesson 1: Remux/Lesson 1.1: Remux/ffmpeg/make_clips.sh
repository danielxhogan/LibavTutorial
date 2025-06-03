#!/bin/zsh
source ~/.zshrc

# # video: h.264, audio: mp3
# taskset -c 0-9 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 23 -tune grain -g 120 \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="h264_mp3.mp4" "./videos/inputs/h264_mp3.mp4"

# # video: h.265, audio: mp3
# taskset -c 1-10 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx265 -preset veryslow -crf 28 -tune grain -g 120 \
#   -x265-params pools=10 \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="h265_mp3.mp4" "./videos/inputs/h265_mp3.mp4"

# video: av1, audio: mp3
# taskset -c 0-9 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v copy \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="av1_mp3.mp4" "./videos/inputs/av1_mp3.mp4"


# # video: h.264, audio: aac
# taskset -c 0-9 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 23 -tune grain -g 120 \
#   -map 0:a:0 -c:a libfdk_aac -b:a 640k -ac 6 \
#   -map_chapters -1 \
#   -metadata title="h264_aac.mkv" "./videos/inputs/h264_aac.mkv"

# # video: h.264, audio: ac3
# taskset -c 0-9 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 23 -tune grain -g 120 \
#   -map 0:a:0 -c:a ac3 -b:a 640k -ac 6 \
#   -map_chapters -1 \
#   -metadata title="h264_ac3.mkv" "./videos/inputs/h264_ac3.mkv"

# # video: h.264, audio: eac3
# taskset -c 0-9 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 23 -tune grain -g 120 \
#   -map 0:a:0 -c:a eac3 -b:a 640k -ac 6 \
#   -map_chapters -1 \
#   -metadata title="h264_eac3.mkv" "./videos/inputs/h264_eac3.mkv"

# # video: h.264, audio: flac
# taskset -c 0,7-15 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 25 -tune grain -g 120 \
#   -map 0:a:0 -c:a flac -b:a 640k -ac 6 \
#   -map_chapters -1 \
#   -metadata title="h264_flac.mkv" "./videos/inputs/h264_flac.mkv"


# # video: vp9, audio: opus
# taskset -c 2-11 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libvpx-vp9 -crf 25 -quality good -g 120 \
#   -map 0:a:0 -c:a libopus -b:a 640k -ac 6 \
#   -map_chapters -1 \
#   -metadata title="vp9_opus.webm" "./videos/inputs/vp9_opus.webm"


# video: theora, audio: vorbis
taskset -c 2-11 ffmpeg \
  -i "./videos/inputs/ikkf.mkv" \
  -map 0:v:0 -c:v libtheora -q:v 7 -g 120 \
  -map 0:a:0 -c:a libvorbis -b:a 640k -ac 6 \
  -map_chapters -1 \
  -metadata title="theora_vorbis.ogg" "./videos/inputs/theora_vorbis.ogg"


# # video: h.264, audio: mp3
# taskset -c 2-11 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 23 -tune grain -g 120 \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="h264_mp3.mov" "./videos/inputs/h264_mp3.mov"

# # video: h.264, audio: mp3
# taskset -c 3-12 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 23 -tune grain -g 120 \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="h264_mp3.wmv" "./videos/inputs/h264_mp3.wmv"

# # video: h.264, audio: mp3
# taskset -c 4-13 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 23 -tune grain -g 120 \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="h264_mp3.avi" "./videos/inputs/h264_mp3.avi"

# # video: h.264, audio: mp3
# taskset -c 5-14 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 23 -tune grain -g 120 \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="h264_mp3.flv" "./videos/inputs/h264_mp3.flv"


# # Make og master video clip
# taskset -c 6-10 $HOME/ffmpeg/bin/ffmpeg \
#   -ss 49:02 \
#   -i "/media/hugexjackedman/share/movies/The Matrix Trilogy/The Matrix/fmt/The Matrix.mkv" \
#   -t 11 \
#   -map 0:v:0 -c:v copy \
#   -map 0:a:1 -c:a copy \
#   -map_chapters -1 \
#   -metadata title="ikkf.mkv" "./videos/inputs/ikkf.mkv"