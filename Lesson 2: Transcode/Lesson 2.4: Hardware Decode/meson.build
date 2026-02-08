project('Hardware Decode', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lsws = dependency('libswscale')
lavu = dependency('libavutil')

executable('hwdec', ['hwdec.c'], dependencies: [lavf, lavc, lsws, lavu])
