project('Hardware Decode 1 Input 1 Output', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavflt = dependency('libavfilter')
lavu = dependency('libavutil')

executable('hwdec1in1out', ['hwdec1in1out.c'], dependencies: [lavf, lavc, lavflt, lavu])