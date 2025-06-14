project('Lesson 1.4 - Add Subtitles', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')

executable('addsubs', ['addsubs.c'], dependencies: [lavf, lavc, lavu])
