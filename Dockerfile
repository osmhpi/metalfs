FROM ubuntu:bionic

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libcxl-dev \
    libfuse-dev \
    liblmdb-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libjq-dev \
 && rm -rf /var/lib/apt/lists/*
ENV DEBIAN_FRONTEND=

WORKDIR /metal

ENV SNAP_ROOT=/metal/snap
RUN git clone https://github.com/open-power/snap $SNAP_ROOT
RUN cd snap && make software

ADD third_party/ third_party/
ADD cmake/ cmake/

ADD CMakeLists.txt .
ADD test/ test/
ADD src/ src/
ADD example/ example/

RUN mkdir build && cd build && cmake ..
RUN make -C build -j8

ENTRYPOINT /metal/build/metal_fs
