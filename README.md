rapidapp framework library
========

a rapid platform-crossed c++ application server framework which is based on libevent.
I hoped that anyone who used rapidapp could build up server rapidly.

DEPENDENCE
- libevent: 2.0.21-stable
- protobuf: 2.5.0
- gflags:   2.1.1
- glog:     0.3.3


Features
========
- Fast, Easy, Cross-Platform I/O Event reactor
- Timer Management
- Backdoor Support
- Standard Start/Stop/Reload Process-Management
- Async RPC Support based on ProtoBuf Message Reflection


connector
========
a connector daemon based on rapidapp, including client api and server api.
it uses protobuf to serialize and deserialize.

Features
========
- client connection management
- speed limit
- overload defend
- encryption & authentication
- stateless & distribution support

Examples
========
- see sample/ for server development
- see server/connector/client_api and server/connector/server_api

How To Compile
========
- python and scons are needed
- run command 'scons -u' to compile, more pls run command 'scons -uh'
