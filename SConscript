import os

debug = ARGUMENTS.get('debug', 1)
Help('this is a platform-cross builder\n  debug=[0|1]')

env = Environment(CCFLAGS='-Werror -g -pg',
                 LIBPATH=['/usr/local/libevent/lib', './'],
                 CPPPATH=['/usr/local/libevent/include/', './'])
#LINKFLAGS
if int(debug) != 0:
    env.Append(CPPDEFINES='_DEBUG')

frame_src = Glob('*.cpp')

env.SharedLibrary('rapidapp', frame_src)
env.StaticLibrary('rapidapp', frame_src)

sample_src = Glob('sample/*.cpp')
sample_src += Glob('./librapidapp.a')
env.Program('sample/app_demo', sample_src, LIBS='event')
