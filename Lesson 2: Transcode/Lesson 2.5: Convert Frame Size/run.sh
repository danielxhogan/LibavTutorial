# # transcode to aac
# # ****************
# # Will not transcode any vidoes, sample formats not supported.
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_aac_20.mkv" "libfdk_aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_aac_51.mkv" "libfdk_aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_truehd_71.mkv" "../videos/outputs/heretic_truehd_to_aac_71.mkv" "libfdk_aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_aac_20.mkv" "../videos/outputs/ll_aac_to_aac_20.mkv" "libfdk_aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_aac_51.mkv" "libfdk_aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_aac_20.mkv" "libfdk_aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/noes_hdma_20.mkv" "../videos/outputs/noes_hdma_to_aac_20.mkv" "libfdk_aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/whiplash_hdma_51.mkv" "../videos/outputs/whiplash_hdma_to_aac_51.mkv" "libfdk_aac"
# cd ..

# # transcode to flac
# # *******************
# # Only transcodes Truehd. All other files have unsupported sample formats.
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_flac_20.mkv" "flac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_flac_51.mkv" "flac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_truehd_71.mkv" "../videos/outputs/heretic_truehd_to_flac_71.mkv" "flac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_aac_20.mkv" "../videos/outputs/ll_aac_to_flac_20.mkv" "flac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_flac_51.mkv" "flac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_flac_20.mkv" "flac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/noes_hdma_20.mkv" "../videos/outputs/noes_hdma_to_flac_20.mkv" "flac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/whiplash_hdma_51.mkv" "../videos/outputs/whiplash_hdma_to_flac_51.mkv" "flac"
# cd ..

# # transcode to opus
# # *****************
# # Will not transcode any videos, sample formats not supported.
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_opus_20.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_opus_51.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_truehd_71.mkv" "../videos/outputs/heretic_truehd_to_opus_71.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_aac_20.mkv" "../videos/outputs/ll_aac_to_opus_20.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_opus_51.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_opus_20.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/noes_hdma_20.mkv" "../videos/outputs/noes_hdma_to_opus_20.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/whiplash_hdma_51.mkv" "../videos/outputs/whiplash_hdma_to_opus_51.mkv" "libopus"
# cd ..

# # transcode to eac3
# # *****************
# # Will not transcode Truehd or HDMA, sample formats not supported.
# # Transcoding from AAC or DTS results in clicks & pops
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_eac3_20.mkv" "eac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_eac3_51.mkv" "eac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_truehd_71.mkv" "../videos/outputs/heretic_truehd_to_eac3_71.mkv" "eac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_aac_20.mkv" "../videos/outputs/ll_aac_to_eac3_20.mkv" "eac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_eac3_51.mkv" "eac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_eac3_20.mkv" "eac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/noes_hdma_20.mkv" "../videos/outputs/noes_hdma_to_eac3_20.mkv" "eac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/whiplash_hdma_51.mkv" "../videos/outputs/whiplash_hdma_to_eac3_51.mkv" "eac3"
# cd ..

# # transcode to ac3
# # ****************
# # Will not transcode Truehd or HDMA, sample formats not supported.
# # Transcoding from AAC results in clicks & pops
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_ac3_20.mkv" "ac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_ac3_51.mkv" "ac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_truehd_71.mkv" "../videos/outputs/heretic_truehd_to_ac3_71.mkv" "ac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_aac_20.mkv" "../videos/outputs/ll_aac_to_ac3_20.mkv" "ac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_ac3_51.mkv" "ac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_ac3_20.mkv" "ac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/noes_hdma_20.mkv" "../videos/outputs/noes_hdma_to_ac3_20.mkv" "ac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/whiplash_hdma_51.mkv" "../videos/outputs/whiplash_hdma_to_ac3_51.mkv" "ac3"
# cd ..

# # transcode to vorbis
# # *******************
# # Will not transcode Truehd or HDMA, sample formats not supported.
# # Will not transcode DTS, error unkown.
# # Transcoding from AC3 or AAC results in distorted audio.
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_vorbis_20.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_vorbis_51.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_truehd_71.mkv" "../videos/outputs/heretic_truehd_to_vorbis_71.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_aac_20.mkv" "../videos/outputs/ll_aac_to_vorbis_20.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_vorbis_51.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_vorbis_20.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/noes_hdma_20.mkv" "../videos/outputs/noes_hdma_to_vorbis_20.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/whiplash_hdma_51.mkv" "../videos/outputs/whiplash_hdma_to_vorbis_51.mkv" "libvorbis"
# cd ..

# # transcode to mp3
# # ****************
# # Will not transcode 5.1, channel layout not supported.
# # Will not transcode Truehd, sample format not supported.
# # Transcoding to all other codecs results in click & pops and/or distortion.
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_mp3_20.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_mp3_51.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_truehd_71.mkv" "../videos/outputs/heretic_truehd_to_mp3_71.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_aac_20.mkv" "../videos/outputs/ll_aac_to_mp3_20.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_mp3_51.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_mp3_20.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/noes_hdma_20.mkv" "../videos/outputs/noes_hdma_to_mp3_20.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/whiplash_hdma_51.mkv" "../videos/outputs/whiplash_hdma_to_mp3_51.mkv" "libmp3lame"
# cd ..


cd build && ninja && ./fsctranscode
