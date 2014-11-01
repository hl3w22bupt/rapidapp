import platform
import os

env = Environment(CCFLAGS='-g -pg',
                 LIBPATH=['/usr/local/lib', '/usr/local/libevent/lib', '/usr/local/protoc/lib', './', '/usr/local/lib'],
                 CPPPATH=['/usr/local/include', '/usr/local/libevent/include/', '/usr/local/protoc/include/',
                 './', 'thirdparty/rapidjson/include'])
sys = platform.system()
if sys != "Darwin":
    env.Append(CCFLAGS=' -Werror')
else:
    env.Append(CCFLAGS=' -stdlib=libstdc++ -Wno-error=c++11-narrowing')
    env.Append(CPPDEFINES=["__IOS__", "_XOPEN_SOURCE", "GFLAGS_NS_GOOGLE"])

Export('env')
SConscript('SConscript')
