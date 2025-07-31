project('Resample Audio', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')
lswr = dependency('libswresample')

executable('swrtranscode', ['swrtranscode.c'], dependencies: [lavf, lavc, lavu, lswr])
