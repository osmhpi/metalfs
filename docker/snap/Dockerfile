ARG VIVADO_EDITION=webpack
FROM metalfs/snap-base:$VIVADO_EDITION

WORKDIR /frameworks

ENV SNAP_ROOT=/frameworks/snap

RUN git clone https://github.com/rs22/snap -b webpack_sim $SNAP_ROOT

RUN cd $SNAP_ROOT && make software
