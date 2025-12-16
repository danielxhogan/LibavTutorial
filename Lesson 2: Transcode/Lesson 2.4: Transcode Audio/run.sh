# cd build && ninja && ./atranscode "../videos/inputs/ll_aac.mkv" "../videos/outputs/ll_aac.mkv" "aac"
# cd ..
# cd build && ninja && ./atranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_ac3_51.mkv" "ac3"
# cd ..
# cd build && ninja && ./atranscode "../videos/inputs/heretic_ac3_20.mkv" "../videos/outputs/heretic_ac3_20.mkv" "ac3"
# cd ..


# if I run this command to transcode an ac3 file into aac:

# cd build && ninja && ./atranscode "../videos/inputs/heretic_ac3_51.mkv" "../videos/outputs/heretic_aac_51.mkv" "aac"

# I get this error:

# nb_samples (1536) > frame_size (1024)


# if I run this command to transcode an aac file into ac3:

# cd build && ninja && ./atranscode "../videos/inputs/ll_aac.mkv" "../videos/outputs/ll_ac3.mkv" "ac3"

# I get this error:

# frame_size (1536) was not respected for a non-last frame

# The standard frame size for ac3 is greater than for aac. In order to transcode from ac3 to aac, or vice versa,
# we'll need to maintain buffers in order to write the correct number of samples into a frame

# This will be covered in the next lesson


cd build && ninja && ./atranscode
