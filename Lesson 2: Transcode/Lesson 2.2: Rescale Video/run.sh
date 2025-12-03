# cd build && ninja && ./swstranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/ll_x265.mkv" "1280" "720" "yuv420p10" "libx265"
# cd ..
# cd build && ninja && ./swstranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_x265.mkv" "1920" "1080" "yuv420p10le" "libx265"

cd build && ninja && ./swstranscode 
