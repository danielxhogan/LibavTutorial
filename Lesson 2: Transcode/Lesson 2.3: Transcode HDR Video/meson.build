project('Transcode HDR Video', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')

executable('hdrtranscode', ['hdrtranscode.c'], dependencies: [lavf, lavc, lavu])
