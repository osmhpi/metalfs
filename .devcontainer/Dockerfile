FROM xilinx-vivado:2018.1

RUN echo "source /opt/Xilinx/Vivado/2018.1/settings64.sh" >> /root/.bashrc

RUN apt-get update && apt-get install -y \
    vim \
    curl \
    build-essential \
    cmake \
    git \
    jq \
    screen \
    libncurses-dev \
    libfuse-dev \
    liblmdb-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libjq-dev \
    libxtst6 \
    libgtk2.0-0 \
    libcanberra-gtk-module

WORKDIR /metal

ENV PSLSE_ROOT=/metal/pslse
RUN git clone https://github.com/ibm-capi/pslse $PSLSE_ROOT
ENV PSLVER=8
ENV VPI_USER_H_DIR=/opt/Xilinx/Vivado/2018.1/data/xsim/include
RUN cd $PSLSE_ROOT && make

ENV SNAP_ROOT=/metal/snap
RUN git clone -b sim_screen https://github.com/rs22/snap $SNAP_ROOT
RUN cd $SNAP_ROOT && make software

# Set the default shell to bash instead of sh
ENV SHELL /bin/bash

RUN echo "export XILINXD_LICENSE_FILE=/tmp/null" >> /root/.bashrc
RUN echo "export PSLSE_ROOT=$PSLSE_ROOT" >> /root/.bashrc
RUN echo "export SNAP_ROOT=$SNAP_ROOT" >> /root/.bashrc
RUN echo "export LC_ALL=C" >> /root/.bashrc
