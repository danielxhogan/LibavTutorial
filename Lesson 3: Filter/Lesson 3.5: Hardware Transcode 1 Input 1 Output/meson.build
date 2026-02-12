project('Hardware Transcode 1 Input 1 Output', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavflt = dependency('libavfilter')
lavu = dependency('libavutil')

executable('hwtc1in1out', ['hwtc1in1out.c'], dependencies: [lavf, lavc, lavflt, lavu])