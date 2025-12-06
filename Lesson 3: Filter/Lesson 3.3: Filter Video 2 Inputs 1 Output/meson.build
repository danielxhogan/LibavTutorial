project('Filter Video 2 Inputs 1 Output', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavflt = dependency('libavfilter')
lsws = dependency('libswscale')
lavu = dependency('libavutil')

executable('fv2in1out', ['fv_2in_1out.c'], dependencies: [lavf, lavc, lavflt, lsws, lavu])
