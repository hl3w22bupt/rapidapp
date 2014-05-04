import os

frame_src = Glob('*.cpp')
SharedLibrary('rapidapp',
              frame_src,
              LIBPATH=['/usr/local/libevent/lib'],
              CPPPATH=['/usr/local/libevent/include/'],
              CCFLAGS='-Werror -g -pg')
