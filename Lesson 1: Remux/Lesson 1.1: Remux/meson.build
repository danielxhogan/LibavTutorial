project('Lesson 1.1 - Remux', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')

executable('remux', ['remux.c'], dependencies: [lavf, lavc, lavu])
