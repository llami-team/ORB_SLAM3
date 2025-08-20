FROM ubuntu:20.04

RUN mkdir /orb_slam3
WORKDIR /orb_slam3

RUN apt update -y && apt upgrade -y
RUN apt install -y git

# clone all for caching
RUN git clone --recursive https://github.com/stevenlovegrove/Pangolin.git

ARG DEBIAN_FRONTEND=noninteractive

RUN apt install -y \
    build-essential python3-opencv libeigen3-dev libc++-dev libepoxy-dev libglew-dev libeigen3-dev cmake g++ ninja-build \
    libgl1-mesa-dev libwayland-dev libxkbcommon-dev wayland-protocols libegl1-mesa-dev \
    libboost-serialization-dev libssl-dev libopencv-dev libgstrtspserver-1.0-dev \
    python3-dev python3-pip

RUN pip3 install setuptools

RUN cd Pangolin && \
    cmake -B build && cmake --build build && \
    cd build && make install

CMD /bin/bash
# build: docker build -t orbslam3 .
# run: docker run --rm -it --entrypoint bash -v .:/orb_slam3/ORB_SLAM3 orbslam3
