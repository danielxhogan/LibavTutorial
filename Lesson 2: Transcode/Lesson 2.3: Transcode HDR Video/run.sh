# cd build && ninja && ./hdrtranscode "../videos/inputs/her_dovi.mkv" "../videos/outputs/her_dovi_svtav1.mkv" "libsvtav1"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/her_dovi.mkv" "../videos/outputs/her_dovi_x265.mkv" "libx265"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/her_dovi.mkv" "../videos/outputs/her_dovi_x265.mp4" "libx265"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/her_dovi.mkv" "../videos/outputs/her_dovi_x265_params.mkv" "libx265" "preset=ultrafast:crf=45"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/ll_hdr.mkv" "../videos/outputs/ll_hdr_svtav1.mkv" "libsvtav1" "preset=9:crf=45"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/ll_hdr.mkv" "../videos/outputs/ll_hdr_x265.mkv" "libx265" "preset=ultrafast:crf=45"

cd build && ninja && ./hdrtranscode 
