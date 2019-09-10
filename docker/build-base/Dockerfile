ARG VIVADO_EDITION=webpack
FROM metalfs/snap:$VIVADO_EDITION

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    libfuse-dev \
    liblmdb-dev \
    libprotobuf-dev \
    protobuf-compiler \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /frameworks

# libjq
RUN curl -sL https://github.com/stedolan/jq/releases/download/jq-1.5/jq-1.5.tar.gz --output jq-1.5.tar.gz && tar -xzf jq-1.5.tar.gz && rm jq-1.5.tar.gz
RUN cd jq-1.5 && ./configure --with-oniguruma=builtin --disable-maintainer-mode && make LDFLAGS=-all-static && mkdir -p builtin/lib && make install

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

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=

RUN echo "export PSLSE_ROOT=$PSLSE_ROOT" >> /root/.bashrc \
 && echo "export SNAP_ROOT=$SNAP_ROOT" >> /root/.bashrc

# Set the default shell to bash instead of sh
ENV SHELL /bin/bash

CMD tail -f /dev/null
