cd build && ninja && ./fv2in1out "../videos/inputs/heretic.mkv" \
"../videos/outputs/heretic_burned_in_subs.mkv" \
"[in1][in2]overlay[out]" "0" "5" \
"libx265" "crf=10:pools=5"

# cd build && ninja && ./fv2in1out "../videos/inputs/heretic.mkv" \
# "../videos/outputs/heretic_burned_in_subs.mkv" \
# "[in2]format=pix_fmts=yuva420p[s_converted]; \
# [in1][s_converted]overlay=x=(W-w)/2:y=(H-h)/2[out]" \
# "0" "5" \
# "libx265" "crf=10:pools=5"

# cd build && ninja && ./fv2in1out "../videos/inputs/heretic.mkv" "../videos/outputs/heretic_burned_in_subs.mkv" "[in1][in2]overlay[out]" "0" "5" "libx265" "crf=10:pools=5"

# cd build && ninja && ./fv2in1out "../videos/inputs/4k_ll.mkv" "../videos/outputs/4k_ll_burned_in_subs.mkv" "[in1][in2]overlay[out]" "0" "7" "libx265" "crf=10:pools=5"

# cd build && ninja && ./fv1in1out 
