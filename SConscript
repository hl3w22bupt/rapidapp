import os

env = Environment(CCFLAGS='-Werror -g -pg',
                 LIBPATH=['/usr/local/libevent/lib'],
                 CPPPATH=['/usr/local/libevent/include/'])
#LINKFLAGS
frame_src = Glob('*.cpp')

env.SharedLibrary('rapidapp', frame_src)
env.StaticLibrary('rapidapp', frame_src)
