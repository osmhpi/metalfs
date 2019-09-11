ARG VIVADO_EDITION=webpack
FROM metalfs/xilinx-vivado:2018.1-$VIVADO_EDITION

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    vim \
    curl ca-certificates \
    build-essential \
    git \
    xterm \
    tmux \
    libncurses-dev \
 && rm -rf /var/lib/apt/lists/*

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=

WORKDIR /frameworks

# PSLSE
ENV PSLSE_ROOT=/frameworks/pslse
ENV PSLVER=8
ENV VPI_USER_H_DIR=/opt/Xilinx/Vivado/2018.1/data/xsim/include
RUN mkdir $PSLSE_ROOT && curl -sL https://github.com/ibm-capi/pslse/archive/v4.1.tar.gz | tar xvz -C $PSLSE_ROOT --strip 1 && cd $PSLSE_ROOT && make

ENV LC_ALL=C
ENV XILINXD_LICENSE_FILE=/dev/null

RUN echo ". /opt/Xilinx/Vivado/2018.1/settings64.sh" >> ~/.snap_profile

# Load .snap_profile for non-interactive bash shells
ENV BASH_ENV=/root/.snap_profile

# Load .snap_profile for interactive bash shells
RUN echo ". ~/.snap_profile" >> ~/.bashrc
