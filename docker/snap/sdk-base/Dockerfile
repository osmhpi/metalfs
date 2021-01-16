ARG VIVADO_EDITION=webpack
FROM metalfs/xilinx-vivado:2018.1-$VIVADO_EDITION

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

# Compilers etc.
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    vim \
    curl ca-certificates \
    build-essential \
    git openssh-client \
    jq \
 && rm -rf /var/lib/apt/lists/*

# Metal FS Dependencies
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    libfuse-dev \
    liblmdb-dev \
    libprotobuf-dev \
    protobuf-compiler \
 && rm -rf /var/lib/apt/lists/*

# CMake (newer than version available from package sources)
RUN mkdir /tmp/cmake \
 && curl -sL https://cmake.org/files/v3.16/cmake-3.16.1-Linux-x86_64.tar.gz | tar xvz -C /tmp/cmake --strip 1 \
 && cp -rf /tmp/cmake/bin /usr/ \
 && cp -rf /tmp/cmake/share /usr/ \
 && rm -rf /tmp/cmake

# GCC 7
RUN apt-key adv --recv-keys --keyserver keyserver.ubuntu.com 60C317803A41BA51845E371A1E9377A2BA9EF27F \
 && printf "deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu xenial main\ndeb-src http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu xenial main" > /etc/apt/sources.list.d/backports.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends gcc-7 g++-7 \
 && rm -rf /var/lib/apt/lists/* \
 && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7

# node.js and npm
RUN curl -sL https://deb.nodesource.com/setup_12.x | bash - \
 && apt-get install -y --no-install-recommends nodejs \
 && rm -rf /var/lib/apt/lists/*

# Prerequisites for SNAP
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    xterm \
    tmux \
    libncurses-dev \
 && rm -rf /var/lib/apt/lists/*

# Documentation
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    python3-pip \
    doxygen \
    graphviz \
 && rm -rf /var/lib/apt/lists/*
COPY docs/requirements.txt /sdk/metal_fs/docs/requirements.txt
RUN pip3 install -r /sdk/metal_fs/docs/requirements.txt

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=

ENV LC_ALL=C
ENV XILINXD_LICENSE_FILE=/dev/null

RUN echo ". /opt/Xilinx/Vivado/2018.1/settings64.sh" >> ~/.metal_profile

# Load .metal_profile for non-interactive bash shells
ENV BASH_ENV=/root/.metal_profile

# Load .metal_profile for interactive bash shells
RUN echo ". ~/.metal_profile" >> ~/.bashrc

ENTRYPOINT []
