ARG VIVADO_EDITION=webpack
ARG PSL=PSL8

FROM metalfs/sdk-base:$VIVADO_EDITION AS build
ARG PSL

ADD . /src/metal_fs
RUN cd /src/metal_fs \
 && mkdir build \
 && cd build \
 && cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPTION_BUILD_TESTS=OFF \
    -DPSL_VERSION=$PSL \
    .. \
 && make && make install

FROM metalfs/sdk-base:$VIVADO_EDITION

COPY --from=build /usr/local/bin/metal-driver /usr/local/bin/
COPY --from=build /usr/local/lib/libmetal-*.so /usr/local/lib/
COPY --from=build /usr/local/share/metal_fs /usr/local/share/metal_fs

RUN ldconfig

ADD . /sdk/metal_fs
ENV METAL_ROOT=/sdk/metal_fs
