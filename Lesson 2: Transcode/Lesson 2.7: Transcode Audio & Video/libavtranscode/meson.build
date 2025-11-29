lib_sources = ['avtranscode.c', 'input.c', 'output.c', 'hdr.c', 'fsc.c']
lib_include_dirs = include_directories('.')

lavf = dependency('libavformat')
lavc = dependency('libavcodec')
lavu = dependency('libavutil')
lswr = dependency('libswresample')

libavtranscode_static_lib = static_library('avtranscode', lib_sources, include_directories: lib_include_dirs, dependencies: [lavf, lavc, lavu, lswr])
libavtranscode = declare_dependency(link_with: libavtranscode_static_lib, include_directories: lib_include_dirs, dependencies: [lavf, lavc, lavu, lswr])