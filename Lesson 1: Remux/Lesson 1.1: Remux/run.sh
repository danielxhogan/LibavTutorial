cd build && ninja && ./remux "../videos/inputs/h264_mp3.mp4" "../videos/outputs/h264_mp3.mkv"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.mp4" "../videos/outputs/h264_mp3.mov"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.mp4" "../videos/outputs/h264_mp3.wmv"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.mp4" "../videos/outputs/h264_mp3.flv"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.mp4" "../videos/outputs/h264_mp3.webm"    # Neither codecs supported in webm
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.mp4" "../videos/outputs/h264_mp3.ogg"     # Neither codecs supported in ogg

# cd build && ninja && ./remux "../videos/inputs/h265_mp3.mp4" "../videos/outputs/h265_mp3.mkv"
# cd build && ninja && ./remux "../videos/inputs/h265_mp3.mp4" "../videos/outputs/h265_mp3.mov"
# cd build && ninja && ./remux "../videos/inputs/h265_mp3.mp4" "../videos/outputs/h265_mp3.flv"
# cd build && ninja && ./remux "../videos/inputs/h265_mp3.mp4" "../videos/outputs/h265_mp3.wmv"   # Failed to write header

# cd build && ninja && ./remux "../videos/inputs/av1_mp3.mp4" "../videos/outputs/av1_mp3.mkv"
# cd build && ninja && ./remux "../videos/inputs/av1_mp3.mp4" "../videos/outputs/av1_mp3.wmv"
# cd build && ninja && ./remux "../videos/inputs/av1_mp3.mp4" "../videos/outputs/av1_mp3.flv"
# cd build && ninja && ./remux "../videos/inputs/av1_mp3.mp4" "../videos/outputs/av1_mp3.mov"     # av1 not supported in mov

# cd build && ninja && ./remux "../videos/inputs/h264_aac.mkv" "../videos/outputs/h264_aac.mp4"
# cd build && ninja && ./remux "../videos/inputs/h264_aac.mkv" "../videos/outputs/h264_aac.mov"
# cd build && ninja && ./remux "../videos/inputs/h264_aac.mkv" "../videos/outputs/h264_aac.flv"
# cd build && ninja && ./remux "../videos/inputs/h264_aac.mkv" "../videos/outputs/h264_aac.wmv"

# cd build && ninja && ./remux "../videos/inputs/h264_ac3.mkv" "../videos/outputs/h264_ac3.mp4"
# cd build && ninja && ./remux "../videos/inputs/h264_ac3.mkv" "../videos/outputs/h264_ac3.mov"
# cd build && ninja && ./remux "../videos/inputs/h264_ac3.mkv" "../videos/outputs/h264_ac3.flv"
# cd build && ninja && ./remux "../videos/inputs/h264_ac3.mkv" "../videos/outputs/h264_ac3.wmv"

# cd build && ninja && ./remux "../videos/inputs/h264_eac3.mkv" "../videos/outputs/h264_eac3.mp4"
# cd build && ninja && ./remux "../videos/inputs/h264_eac3.mkv" "../videos/outputs/h264_eac3.mov"
# cd build && ninja && ./remux "../videos/inputs/h264_eac3.mkv" "../videos/outputs/h264_eac3.wmv"
# cd build && ninja && ./remux "../videos/inputs/h264_eac3.mkv" "../videos/outputs/h264_eac3.flv"   # No audio playback

# cd build && ninja && ./remux "../videos/inputs/h264_flac.mkv" "../videos/outputs/h264_flac.mp4"
# cd build && ninja && ./remux "../videos/inputs/h264_flac.mkv" "../videos/outputs/h264_flac.mov"   # flac not supported in mov
# cd build && ninja && ./remux "../videos/inputs/h264_flac.mkv" "../videos/outputs/h264_flac.wmv"
# cd build && ninja && ./remux "../videos/inputs/h264_flac.mkv" "../videos/outputs/h264_flac.flv"   # No audio playback

# cd build && ninja && ./remux "../videos/inputs/vp9_opus.webm" "../videos/outputs/vp9_opus.mp4"
# cd build && ninja && ./remux "../videos/inputs/vp9_opus.webm" "../videos/outputs/vp9_opus.mkv"
# cd build && ninja && ./remux "../videos/inputs/vp9_opus.webm" "../videos/outputs/vp9_opus.mov"    # vp9 not supported in mov
# cd build && ninja && ./remux "../videos/inputs/vp9_opus.webm" "../videos/outputs/vp9_opus.wmv"    # Failed to write header
# cd build && ninja && ./remux "../videos/inputs/vp9_opus.webm" "../videos/outputs/vp9_opus.flv"    # No audio playback

# cd build && ninja && ./remux "../videos/inputs/theora_vorbis.ogg" "../videos/outputs/theora_vorbis.mp4"   # theora not supported in mp4
# cd build && ninja && ./remux "../videos/inputs/theora_vorbis.ogg" "../videos/outputs/theora_vorbis.mkv"
# cd build && ninja && ./remux "../videos/inputs/theora_vorbis.ogg" "../videos/outputs/theora_vorbis.mov"
# cd build && ninja && ./remux "../videos/inputs/theora_vorbis.ogg" "../videos/outputs/theora_vorbis.wmv"
# cd build && ninja && ./remux "../videos/inputs/theora_vorbis.ogg" "../videos/outputs/theora_vorbis.flv"   # theora not supported in flv

# cd build && ninja && ./remux "../videos/inputs/h264_mp3.mov" "../videos/outputs/h264_mp3.mp4"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.mov" "../videos/outputs/h264_mp3.mkv"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.mov" "../videos/outputs/h264_mp3.wmv"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.mov" "../videos/outputs/h264_mp3.flv"

# cd build && ninja && ./remux "../videos/inputs/h264_mp3.flv" "../videos/outputs/h264_mp3.mp4"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.flv" "../videos/outputs/h264_mp3.mkv"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.flv" "../videos/outputs/h264_mp3.mov"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.flv" "../videos/outputs/h264_mp3.wmv"

# cd build && ninja && ./remux "../videos/inputs/h264_mp3.wmv" "../videos/outputs/h264_mp3.mp4"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.wmv" "../videos/outputs/h264_mp3.mkv"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.wmv" "../videos/outputs/h264_mp3.mov"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.wmv" "../videos/outputs/h264_mp3.flv"

# cd build && ninja && ./remux "../videos/inputs/h264_mp3.avi" "../videos/outputs/h264_mp3.mp4"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.avi" "../videos/outputs/h264_mp3.mkv"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.avi" "../videos/outputs/h264_mp3.mov"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.avi" "../videos/outputs/h264_mp3.wmv"
# cd build && ninja && ./remux "../videos/inputs/h264_mp3.avi" "../videos/outputs/h264_mp3.flv"


# cd build && ninja && ./remux