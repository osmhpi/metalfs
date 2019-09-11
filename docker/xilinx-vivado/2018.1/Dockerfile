FROM ubuntu:xenial-20190720

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    libglib2.0-0 \
    libsm6 \
    libxi6 \
    libxrender1 \
    libxrandr2 \
    libfreetype6 \
    libfontconfig \
    libxtst6 \
    libgtk2.0-0 \
    libcanberra-gtk-module \
 && rm -rf /var/lib/apt/lists/*

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=
