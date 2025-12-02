# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_svtav1.mp4" "libsvtav1"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_svtav1_params.mp4" "libsvtav1" "film-grain=20:crf=15:tune=0:preset=8:keyint=48:tile-rows=2:tile-columns=2:fast-decode=2"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_svtav1_params.mkv" "libsvtav1" "film-grain=20:crf=15:tune=0:preset=8:keyint=48:tile-rows=2:tile-columns=2:fast-decode=2"
# cd ..

# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_x264.mp4" "libx264"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_x264_params.mp4" "libx264" "crf=45"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_x264_params.mkv" "libx264" "crf=45"
# cd ..

# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_x265.mp4" "libx265"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_x265_params.mp4" "libx265" "crf=20:tune=grain"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll_x265_params.mkv" "libx265" "crf=20:tune=grain"
# cd ..


# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_svtav1.mp4" "libsvtav1"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_svtav1_params.mp4" "libsvtav1" "film-grain=20:crf=15:tune=0:preset=8:keyint=48:tile-rows=2:tile-columns=2:fast-decode=2"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_svtav1_params.mkv" "libsvtav1" "film-grain=20:crf=15:tune=0:preset=8:keyint=48:tile-rows=2:tile-columns=2:fast-decode=2"
# cd ..

# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_x264.mp4" "libx264"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_x264_params.mp4" "libx264" "crf=45"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_x264_params.mkv" "libx264" "crf=45"
# cd ..

# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_x265.mp4" "libx265"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_x265_params.mp4" "libx265" "crf=20:tune=grain"
# cd ..
# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_x265_params.mkv" "libx265" "crf=20:tune=grain"
# cd ..


# [h263 @ 0x5c67b80cc340] The specified picture size of 1920x1080 is not valid for the H.263 codec.
# Valid sizes are 128x96, 176x144, 352x288, 704x576, and 1408x1152. Try H.263+.

# cd build && ninja && ./vtranscode "../videos/inputs/ll.mkv" "../videos/outputs/ll.mkv" "h263"


# [h263 @ 0x64fe304a87c0] Specified pixel format yuv420p10le is not supported by the h263 encoder.
# [h263 @ 0x64fe304a87c0] Supported pixel formats:
# [h263 @ 0x64fe304a87c0]   yuv420p

# cd build && ninja && ./vtranscode "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll.mov" "h263"

# cd build && ninja && ./vtranscode 
