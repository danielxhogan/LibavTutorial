project('Filter Video 1 Input 1 Output', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavflt = dependency('libavfilter')
lavu = dependency('libavutil')

executable('fv1in1out', ['fv_1in_1out.c'], dependencies: [lavf, lavc, lavflt, lavu])
