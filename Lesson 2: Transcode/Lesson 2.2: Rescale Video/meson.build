project('Rescale Video', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lsws = dependency('libswscale')
lavu = dependency('libavutil')

executable('swstranscode', ['swstranscode.c'], dependencies: [lavf, lavc, lavu, lsws])
