project('Transcode Audio', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')

executable('atranscode', ['atranscode.c'], dependencies: [lavf, lavc, lavu])
