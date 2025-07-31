project('Convert Frame Size', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')

executable('fsctranscode', ['fsctranscode.c'], dependencies: [lavf, lavc, lavu])
