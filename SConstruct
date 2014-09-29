import platform
import os

env = Environment(CCFLAGS='-Werror -g -pg',
                 LIBPATH=['/usr/local/libevent/lib', '/usr/local/protoc/lib', './', '/usr/local/lib'],
                 CPPPATH=['/usr/local/libevent/include/', '/usr/local/protoc/include/',
                 './', 'thirdparty/rapidjson/include'])
Export('env')

SConscript('SConscript')
