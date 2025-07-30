project('Transcode Video', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')

executable('vtranscode', ['vtranscode.c'], dependencies: [lavf, lavc, lavu])
