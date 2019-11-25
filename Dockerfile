FROM ubuntu:bionic AS build

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    git \
    ca-certificates \
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

RUN mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ..
RUN make -C build -j8


FROM ubuntu:bionic
COPY --from=build /metal/build/metal_fs /usr/bin/metal_fs
COPY --from=build /metal/snap/software/lib/libsnap.so /usr/lib/libsnap.so

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    fuse \
    liblmdb0 \
    libjq1 \
    libprotobuf10 \
    libcxl1 \
 && rm -rf /var/lib/apt/lists/*
ENV DEBIAN_FRONTEND=

ENTRYPOINT /usr/bin/metal_fs
