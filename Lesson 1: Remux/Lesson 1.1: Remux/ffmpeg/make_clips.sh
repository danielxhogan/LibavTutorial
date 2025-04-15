#!/bin/zsh
source ~/.zshrc

# # video: h.264, audio: mp3
# taskset -c 0,7-15 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/og_ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 20 -tune grain -g 48 \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="h4m_ikkf.flv" "./videos/inputs/h4m_ikkf.flv"

# # video: h.264, audio: mp3
# taskset -c 0,7-15 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/og_ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 20 -tune grain -g 48 \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="h4m_ikkf.wmv" "./videos/inputs/h4m_ikkf.wmv"

# # video: h.264, audio: mp3
# taskset -c 0,7-15 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/og_ikkf.mkv" \
#   -map 0:v:0 -c:v libx264 -preset veryslow -crf 20 -tune grain -g 48 \
#   -map 0:a:0 -c:a mp3 -b:a 640k \
#   -map_chapters -1 \
#   -metadata title="h4m_ikkf.avi" "./videos/inputs/h4m_ikkf.avi"

# # video: h.265, audio: aac
# taskset -c 0,7-15 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/og_ikkf.mkv" \
#   -map 0:v:0 -c:v libx265 -preset veryslow -crf 25 -tune grain -g 48 \
#   -x265-params pools=10 \
#   -map 0:a:0 -c:a copy \
#   -map_chapters -1 \
#   -metadata title="h5a_ikkf.mov" "./videos/inputs/h5a_ikkf.mov"

# # video: av1, audio: opus
# taskset -c 6-10 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/og_ikkf.mkv" \
#   -map 0:v:0 -c:v copy \
#   -map 0:a:0 -c:a libopus -b:a 640k -ac 6 \
#   -map_chapters -1 \
#   -metadata title="ao_ikkf.mkv" "./videos/inputs/ao_ikkf.mkv"

# # video: vp9, audio: vorbis
# taskset -c 6,7 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/og_ikkf.mkv" \
#   -map 0:v:0 -c:v libvpx-vp9 -crf 20 -quality good -g 48 \
#   -map 0:a:0 -c:a libvorbis -b:a 640k -ac 6 \
#   -map_chapters -1 \
#   -metadata title="vv_ikkf.webm" "./videos/inputs/vv_ikkf.webm"

# # video: vp9, audio: flac
# taskset -c 6,7 $HOME/ffmpeg/bin/ffmpeg \
#   -i "./videos/inputs/og_ikkf.mkv" \
#   -map 0:v:0 -c:v libvpx-vp9 -crf 20 -quality good -g 48 \
#   -map 0:a:0 -c:a flac -b:a 640k -ac 6 \
#   -map_chapters -1 \
#   -metadata title="vf_ikkf.mp4" "./videos/inputs/vf_ikkf.mp4"

# # video: theora, audio: vorbis
# taskset -c 6,7 ffmpeg \
#   -i "./videos/inputs/og_ikkf.mkv" \
#   -map 0:v:0 -c:v libtheora -q:v 10 -g 48 \
#   -map 0:a:0 -c:a libvorbis -b:a 640k -ac 6 \
#   -map_chapters -1 \
#   -metadata title="tv_ikkf.ogg" "./videos/inputs/tv_ikkf.ogg"

# # Make og master video clip
# taskset -c 6-10 $HOME/ffmpeg/bin/ffmpeg \
#   -ss 49:02 \
#   -i "/media/hugexjackedman/share/movies/The Matrix Trilogy/The Matrix/fmt/The Matrix.mkv" \
#   -t 11 \
#   -map 0:v:0 -c:v copy \
#   -map 0:a:1 -c:a copy \
#   -map_chapters -1 \
#   -metadata title="og_ikkf.mkv" "./videos/inputs/og_ikkf.mkv"