FROM ubuntu:bionic

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcxl-dev \
    libfuse-dev \
    liblmdb-dev

WORKDIR /metal

ENV SNAP_ROOT=/metal/snap
RUN git clone https://github.com/open-power/snap $SNAP_ROOT
RUN cd snap && make software

ADD third_party/ third_party/
ADD cmake/ cmake/

ADD CMakeLists.txt .
ADD test/ test/
ADD src/ src/

RUN mkdir build && cd build && cmake ..
RUN make -C build -j8

ENTRYPOINT /metal/build/metal_fs
