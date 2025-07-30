project('Rescale Video', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')
lsws = dependency('libswscale')

executable('swstranscode', ['swstranscode.c'], dependencies: [lavf, lavc, lavu, lsws])
