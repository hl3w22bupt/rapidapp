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
SConscript('framework/SConscript')

#connector
SConscript('server/connector/SConscript')

#sample
SConscript('sample/SConscript')

#echosvr
os.system('echo \'genereate c++ code over protobuf [echosvr.proto]...\' && cd tools/echosvr && protoc --cpp_out=. echosvr.proto')
tools_echosvr_src = Glob('tools/echosvr/*.cpp')
tools_echosvr_src += Glob('./lib/librapidapp.a')
env.Program('tools/echo_svr', tools_echosvr_src, LIBS=['event', 'glog', 'gflags', 'protobuf', 'pthread'])

#install to sdk
env.Install('sdk/', 'utils/tsocket_util.cpp')
env.Install('sdk/', 'utils/tsocket_util.h')
env.Install('sdk/', 'utils/tcp_socket.c')
env.Install('sdk/', 'utils/tcp_socket.h')
env.Install('sdk/', Glob('server/connector/client_api/cocos_lua/cocos_connector.*'))
env.Install('sdk/', Glob('server/connector/client_api/connector_client_api*.cpp'))
env.Install('sdk/', Glob('server/connector/client_api/connector_client_api*.h'))
