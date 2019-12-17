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
    git \
 && rm -rf /var/lib/apt/lists/*

# Metal FS Libraries
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    cmake \
    libfuse-dev \
    liblmdb-dev \
    libprotobuf-dev \
    protobuf-compiler \
 && rm -rf /var/lib/apt/lists/*

# libjq
RUN curl -sL https://github.com/stedolan/jq/releases/download/jq-1.5/jq-1.5.tar.gz --output /tmp/jq-1.5.tar.gz \
 && (cd /tmp && tar -xzf jq-1.5.tar.gz && rm jq-1.5.tar.gz) \
 && (cd /tmp/jq-1.5 && ./configure --with-oniguruma=builtin --disable-maintainer-mode && make LDFLAGS=-all-static && mkdir -p builtin/lib && make install) \
 && rm -rf /tmp/jq-1.5

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

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=

ENV LC_ALL=C
ENV XILINXD_LICENSE_FILE=/dev/null

RUN echo ". /opt/Xilinx/Vivado/2018.1/settings64.sh" >> ~/.metal_profile

# Load .metal_profile for non-interactive bash shells
ENV BASH_ENV=/root/.metal_profile

# Load .metal_profile for interactive bash shells
RUN echo ". ~/.metal_profile" >> ~/.bashrc

ARG PSLVER=8

# PSLSE and SNAP
RUN mkdir /tmp/pslse \
 && curl -sL https://github.com/ibm-capi/pslse/archive/v4.1.tar.gz | tar xvz -C /tmp/pslse --strip 1 \
 && git clone https://github.com/open-power/snap /tmp/snap \
 && cd /tmp/snap/software \
 && git checkout bc58d6c \
 && PSLVER=$PSLVER make \
 && cp /tmp/snap/software/include/* /usr/local/include/ \
 && cp /tmp/snap/software/lib/*.a /usr/local/lib/ \
 && cp /tmp/snap/software/lib/*.so /usr/local/lib/ \
 && cp /tmp/pslse/libcxl/*.h /usr/local/include/ \
 && cp /tmp/pslse/libcxl/*.a /usr/local/lib/ \
 && cp /tmp/pslse/libcxl/*.so /usr/local/lib/ \
 && rm -rf /tmp/snap \
 && rm -rf /tmp/pslse
