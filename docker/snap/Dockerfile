ARG VIVADO_EDITION=webpack
FROM metalfs/snap-base:$VIVADO_EDITION

WORKDIR /frameworks

# SNAP
ENV SNAP_ROOT=/frameworks/snap

RUN mkdir $SNAP_ROOT \
 && curl -sL https://github.com/open-power/snap/archive/d565a0d.tar.gz | tar xvz -C $SNAP_ROOT --strip 1

RUN cd $SNAP_ROOT && make software
