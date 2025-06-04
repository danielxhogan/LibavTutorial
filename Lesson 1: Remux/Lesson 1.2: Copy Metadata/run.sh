# All metadata, and chapters are copied.
cd build && ninja && ./cpmetadata "../videos/inputs/vp9_opus.webm" "../videos/outputs/vp9_opus.mkv" "vp9_opus.mkv"

# All metadata, and chapters are copied.
# cd build && ninja && ./cpmetadata "../videos/inputs/vp9_opus.mkv" "../videos/outputs/vp9_opus.mkv" "vp9_opus.webm"

# All stream metadata is copied. Ogg does not support file metadata, or traditional chapters.
# cd build && ninja && ./cpmetadata "../videos/inputs/theora_vorbis.mkv" "../videos/outputs/theora_vorbis.ogg" "theora_vorbis.ogg"

# Chapters are copied but only supported metadata values are included. Stream title not supported in mp4.
# cd build && ninja && ./cpmetadata "../videos/inputs/h264_eac3.mkv" "../videos/outputs/h264_eac3.mp4" "h264_eac3.mp4"

# Identical output as above
# cd build && ninja && ./cpmetadata "../videos/inputs/h264_eac3.mkv" "../videos/outputs/h264_eac3.mov" "h264_eac3.mov"

# Chapters and file metadata are copied. Except for language, stream metadata is not supported in wmv.
# cd build && ninja && ./cpmetadata "../videos/inputs/av1_mp3.mp4" "../videos/outputs/av1_mp3.wmv" "av1_mp3.wmv"

# Only file title is copied. Chapters are not supported in avi
# cd build && ninja && ./cpmetadata "../videos/inputs/av1_mp3.mp4" "../videos/outputs/av1_mp3.avi" "av1_mp3.avi"

# Stream title is copied. avi does not support stream language.
# cd build && ninja && ./cpmetadata "../videos/inputs/theora_vorbis.ogg" "../videos/outputs/theora_vorbis.avi" "theora_vorbis.avi"

# File metadata is copied. Chapters and stream metadata is not supported in flv.
# cd build && ninja && ./cpmetadata "../videos/inputs/av1_mp3.mp4" "../videos/outputs/av1_mp3.flv" "av1_mp3.flv"

# cd build && ninja && ./cpmetadata
