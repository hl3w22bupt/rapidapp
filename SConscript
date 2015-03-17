import os
Import('env')

debug = ARGUMENTS.get('debug', 1)
gflags_version = ARGUMENTS.get('gflags_ver', 1)
Help('this is a platform-cross builder\n  debug=[0|1] gflags_ver=[1|2]')

#LINKFLAGS
if int(debug) != 0:
    env.Append(CPPDEFINES='_DEBUG')
else:
    env.Append(CPPDEFINES='NDEBUG')

if int(gflags_version) != 1:
    env.Append(CPPDEFINES='GFLAGS_NS_GOOGLE')

#rapidapp lib
frame_src = Glob('*.cpp')
frame_src += Glob('utils/*.c')
frame_src += Glob('utils/*.cpp')
frame_src += Glob('coroutine/*.cpp')

env.SharedLibrary('rapidapp', frame_src)
env.StaticLibrary('rapidapp', frame_src)

#sample
os.system('echo \'genereate c++ code over protobuf [sample.proto]...\' && cd sample && protoc --cpp_out=. sample.proto')
sample_src = Glob('sample/*.cpp')
sample_src += Glob('sample/*.cc')
sample_src += Glob('./librapidapp.a')
env.Program('sample/app_demo', sample_src, LIBS=['event', 'glog', 'gflags', 'protobuf', 'pthread'])

#echosvr
os.system('echo \'genereate c++ code over protobuf [echosvr.proto]...\' && cd tools/echosvr && protoc --cpp_out=. echosvr.proto')
tools_echosvr_src = Glob('tools/echosvr/*.cpp')
tools_echosvr_src += Glob('./librapidapp.a')
env.Program('tools/echo_svr', tools_echosvr_src, LIBS=['event', 'glog', 'gflags', 'protobuf', 'pthread'])

#connector
SConscript('server/connector/SConscript')

#game server
SConscript('server/ballgame/SConscript')
