import os

debug = ARGUMENTS.get('debug', 1)
Help('this is a platform-cross builder\n  debug=[0|1]')

env = Environment(CCFLAGS='-Werror -g -pg',
                 LIBPATH=['/usr/local/libevent/lib', '/usr/local/protoc/lib', './'],
                 CPPPATH=['/usr/local/libevent/include/', '/usr/local/protoc/include/', './'])
#LINKFLAGS
if int(debug) != 0:
    env.Append(CPPDEFINES='_DEBUG')
else:
    env.Append(CPPDEFINES='NDEBUG')


frame_src = Glob('*.cpp')
frame_src += Glob('utils/*.c')
frame_src += Glob('utils/*.cpp')

env.SharedLibrary('rapidapp', frame_src)
env.StaticLibrary('rapidapp', frame_src)

os.system('echo \'genereate c++ code over protobuf...\' && cd sample && protoc --cpp_out=. sample.proto')
sample_src = Glob('sample/*.cpp')
sample_src += Glob('sample/*.cc')
sample_src += Glob('./librapidapp.a')
env.Program('sample/app_demo', sample_src, LIBS=['event', 'glog', 'gflags', 'protobuf', 'pthread'])
