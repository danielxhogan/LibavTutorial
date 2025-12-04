project('Filter Video 1 Input 2 Outputs', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavflt = dependency('libavfilter')
lavsws = dependency('libswscale')
lavu = dependency('libavutil')

executable('fv1in2out', ['fv_1in_2out.c'], dependencies: [lavf, lavc, lavflt, lavsws, lavu])
