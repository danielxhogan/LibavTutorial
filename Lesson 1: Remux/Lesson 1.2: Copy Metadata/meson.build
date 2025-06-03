project('Lesson 1.2 - Copy Metadata', 'c')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')

executable('cpmetadata', ['cpmetadata.c'], dependencies: [lavf, lavc, lavu])
