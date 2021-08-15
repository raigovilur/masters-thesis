FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Tallinn

COPY lib.tar.gz /usr/local/lib
COPY include.tar.gz /usr/local/include

COPY . /app
WORKDIR /app

RUN mkdir file 

RUN apt -y update && \
    apt install -y software-properties-common lsb-release wget && \
    apt clean all && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" && \
    apt -y update && \
    apt install -y cmake

RUN apt -y install  \
      sudo                      \
      make                      \
      gcc                       \
      g++                       \
      wget                      \
      git                       \
      m4                        \
      libboost-all-dev          \
      libevent-dev              \
      libdouble-conversion-dev  \
      libgoogle-glog-dev        \
      libgflags-dev             \
      libiberty-dev             \
      liblz4-dev                \
      liblzma-dev               \
      libsnappy-dev             \
      zlib1g-dev                \
      binutils-dev              \
      libjemalloc-dev           \
      libssl-dev                \
      pkg-config                \
      libsodium-dev             \
      libbz2-dev                \
      libunwind-dev             \
      iputils-ping

#RUN cd /opt/ && wget https://github.com/Kitware/CMake/releases/download/v3.21.1/cmake-3.21.1.tar.gz && \
#    tar xvzf cmake-3.21.1.tar.gz && \
#    cd cmake-3.21.1 && \
#    mkdir build_ && cd build_ && \
#    cmake .. && \
#    make  && make install && \
#    apt remove -y cmake

RUN apt -y install \ 
      nmap                      \
      iperf3                    \
      vim

RUN wget https://raw.githubusercontent.com/facebookincubator/fizz/master/build/fbcode_builder/CMake/FindSodium.cmake -O /usr/share/cmake-3.21/Modules/FindSodium.cmake

RUN cd /usr/local/lib && tar xvzf /usr/local/lib/lib.tar.gz 
RUN cd /usr/local/include && tar xvzf /usr/local/include/include.tar.gz

RUN cd /app && \
    mkdir build_ && cd build_ && \
    cmake .. && \
    make
