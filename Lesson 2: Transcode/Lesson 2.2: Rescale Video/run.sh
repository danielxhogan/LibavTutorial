# If I try to transcode the video to h263 using the source pixel format using this command:

# cd build && ninja && ./swstranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_h263.mkv" "3840" "2160" "yuv420p10" "h263"

# I get this error:

# [h263 @ 0x5736af10a700] Specified pixel format yuv420p10le is not supported by the h263 encoder.
# [h263 @ 0x5736af10a700] Supported pixel formats:
# [h263 @ 0x5736af10a700]   yuv420p

# If I modify the command to use yuv420p like this:

# cd build && ninja && ./swstranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_h263.mkv" "3840" "2160" "yuv420p" "h263"

# I get this error:

# [h263 @ 0x612950b1a700] H.263 does not support resolutions above 2048x1152

# If I try converting to 1920x1080 with this command:

# cd build && ninja && ./swstranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_h263.mkv" "1920" "1080" "yuv420p" "h263"

# I get this error:

# [h263 @ 0x5a5c06688700] The specified picture size of 1920x1080 is not valid for the H.263 codec.
# Valid sizes are 128x96, 176x144, 352x288, 704x576, and 1408x1152. Try H.263+.

# If I run the command with any of these resolutions, I can now transcode the video to h263:

# cd build && ninja && ./swstranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_h263_128x96.mkv" "128" "96" "yuv420p" "h263"
# cd build && ninja && ./swstranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_h263_176x144.mkv" "176" "144" "yuv420p" "h263"
# cd build && ninja && ./swstranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_h263_352x288.mkv" "352" "288" "yuv420p" "h263"
# cd build && ninja && ./swstranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_h263_704x576.mkv" "704" "576" "yuv420p" "h263"
# cd build && ninja && ./swstranscode "../videos/inputs/noes.mkv" "../videos/outputs/noes_h263_1408x1152.mkv" "1408" "1152" "yuv420p" "h263"

# cd build && ninja && ./swstranscode 
