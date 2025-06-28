project('Lesson 1.6 - Demux', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')

executable('demux', ['demux.c'], dependencies: [lavf, lavc, lavu])
