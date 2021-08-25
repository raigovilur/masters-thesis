FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Tallinn

COPY docker/lib.tar.gz /usr/local/lib
COPY docker/include.tar.gz /usr/local/include

COPY . /app
WORKDIR /app

RUN mkdir out && mkdir out/server

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


RUN apt -y install \ 
      nmap                      \
      iperf3                    \
      vim

RUN wget https://raw.githubusercontent.com/facebookincubator/fizz/master/build/fbcode_builder/CMake/FindSodium.cmake -O /usr/share/cmake-3.21/Modules/FindSodium.cmake

RUN cd /usr/local/lib && tar xvzf /usr/local/lib/lib.tar.gz && rm /usr/local/lib/lib.tar.gz
RUN cd /usr/local/include && tar xvzf /usr/local/include/include.tar.gz && rm /usr/local/include/include.tar.gz

RUN cd /app && \
    mkdir build_ && cd build_ && \
    cmake .. && \
    make

RUN chmod 755 run.sh
