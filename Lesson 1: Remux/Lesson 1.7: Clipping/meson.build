project('Lesson 1.7 - Clipping', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')

executable('clip', ['clip.c'], dependencies: [lavf, lavc, lavu])
