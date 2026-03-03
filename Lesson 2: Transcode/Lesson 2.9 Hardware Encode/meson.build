project('Hardware Encode', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lsws = dependency('libswscale')
lavu = dependency('libavutil')

executable('hwenc', ['hwenc.c'], dependencies: [lavf, lavc, lsws, lavu])
