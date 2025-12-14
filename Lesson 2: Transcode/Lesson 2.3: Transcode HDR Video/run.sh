# # heretic has dolbyvision hdr
# cd build && ninja && ./hdrtranscode "../videos/inputs/heretic.mkv" "../videos/outputs/heretic_dovi_svtav1.mkv" "libsvtav1" "preset=10:crf=30"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/heretic.mkv" "../videos/outputs/heretic_dovi_x265.mkv" "libx265" "crf=30"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/heretic.mkv" "../videos/outputs/heretic_dovi_x265.mp4" "libx265" "crf=30"
# cd ..

# # noes has standard hdr10
# cd build && ninja && ./hdrtranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_hdr_svtav1.mkv" "libsvtav1" "preset=10:crf=30"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_hdr_x265.mkv" "libx265" "crf=30"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_hdr_x265.mp4" "libx265" "crf=30"
# cd ..

# # whiplash has standard hdr10
# cd build && ninja && ./hdrtranscode "../videos/inputs/whiplash.mkv" "../videos/outputs/whiplash_hdr_svtav1.mkv" "libsvtav1" "preset=10:crf=30"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/whiplash.mkv" "../videos/outputs/whiplash_hdr_x265.mkv" "libx265" "crf=30"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/whiplash.mkv" "../videos/outputs/whiplash_hdr_x265.mp4" "libx265" "crf=30"
# cd ..

# ll is standard hd with no hdr
# cd build && ninja && ./hdrtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_sdr_svtav1.mkv" "libsvtav1" "preset=10:crf=30"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_sdr_x265.mkv" "libx265" "crf=30"
# cd ..
# cd build && ninja && ./hdrtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_sdr_x265.mp4" "libx265" "crf=30"
# cd ..


cd build && ninja && ./hdrtranscode 
