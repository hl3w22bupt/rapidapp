#Dockerfile
FROM ubuntu
MAINTAINER hul <hl3w22bupt@gmail.com>
#USER hul
#install dev:base
RUN apt-get install g++
RUN apt-get install vim
RUN apt-get install ctags
RUN apt-get install cscope
RUN apt-get install python
RUN apt-get install scons
RUN apt-get install git
#install deps
RUN apt-get install libprotobuf-dec
RUN apt-get install protobuf-compiler
RUN apt-get install libevent-dev
RUN apt-get install libgflags-dev
RUN apt-get install libgoogle-glog-dev
RUN apt-get install libboost-thread-dev

ENV LANG en_US.UTF-8
ENV LC_ALL en_US.UTF-8
CMD echo "welcome to rapidapp dev"
#ADD ...

#End
