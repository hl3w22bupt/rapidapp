import os
Import('env')

debug = ARGUMENTS.get('debug', 1)
Help('this is a platform-cross builder\n  debug=[0|1]')

#LINKFLAGS
if int(debug) != 0:
    env.Append(CPPDEFINES='_DEBUG')
else:
    env.Append(CPPDEFINES='NDEBUG')

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
os.system('echo \'genereate c++ code over protobuf [client.proto]...\' && cd server/connector && protoc --cpp_out=. client.proto')
os.system('echo \'genereate c++ code over protobuf [server.proto]...\' && cd server/connector && protoc --cpp_out=. server.proto')
os.system('echo \'genereate c++ code over protobuf [config.proto]...\' && cd server/connector && protoc --cpp_out=. config.proto')
connector_src = Glob('server/connector/*.cpp')
connector_src += Glob('server/connector/*.cc')
connector_src += Glob('./librapidapp.a')
env.Program('server/connector_svr', connector_src, LIBS=['event', 'glog', 'gflags', 'protobuf', 'pthread'])

#connector client
clientsrc = Glob('server/connector/client_api/*.cpp')
clientsrc += Glob('server/connector/*.cc')
clientsrc += Glob('utils/rap_net_uri.c')
clientsrc += Glob('./librapidapp.a')
env.Program('server/connector/client_api/connector_client', clientsrc, OBJPREFIX='cclient_', LIBS=['event', 'glog', 'gflags', 'protobuf', 'pthread', 'boost_thread'])

#connector svr api
svrapi_src = Glob('server/connector/server_api/*.cpp')
svrapi_src += Glob('server/connector/server.pb.cc')
env.StaticLibrary('connapi', svrapi_src)

#game server
SConscript('server/ballgame/SConscript')
