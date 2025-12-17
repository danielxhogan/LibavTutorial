# # works fine no issues
# # **********************************
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_aac_51.mkv" "aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_ac3_51.mkv" "ac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_aac_20.mkv" "aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_ac3_20.mkv" "ac3"
# cd ..

# # runs but audio has clicks and pops and/or distortion
# # ****************************************************
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_mp3_20.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_aac_20.mkv" "../videos/outputs/ll_aac_to_ac3_20.mkv" "ac3"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_aac_20.mkv" "../videos/outputs/ll_aac_to_mp3_20.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_aac_20.mkv" "aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_aac_51.mkv" "aac"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_vorbis_20.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_vorbis_51.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_mp3_20.mkv" "libmp3lame"
# cd ..

# # input sample format not supported in output codec
# # *************************************************
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_opus_51.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_opus_20.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_to_opus_20.mkv" "libopus"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_opus_51.mkv" "libopus"
# cd ..

# # input channel layout not supported in output
# # ********************************************
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_mp3_51.mkv" "libmp3lame"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_to_mp3_51.mkv" "libmp3lame"
# cd ..

# # unknown error
# # *************
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_51.mkv" "../videos/outputs/ll_dts_to_vorbis_51.mkv" "libvorbis"
# cd ..
# cd build && ninja && ./fsctranscode "../videos/inputs/ll_dts_20.mkv" "../videos/outputs/ll_dts_to_vorbis_20.mkv" "libvorbis"
# cd ..


cd build && ninja && ./fsctranscode
