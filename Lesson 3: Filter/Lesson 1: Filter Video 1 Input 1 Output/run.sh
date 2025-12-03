# cd build && ninja && ./fv1in1out "../videos/inputs/bttf.mkv" "../videos/outputs/bttf_deinterlaced.mkv" "yadif" "libx265" "crf=10"
# cd ..
# cd build && ninja && ./fv1in1out "../videos/inputs/ll.mkv" "../videos/outputs/ll.mkv" "null" "libx265"
# cd ..
# cd build && ninja && ./fv1in1out "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll.mkv" "bwdif" "libx265"

cd build && ninja && ./fv1in1out 
