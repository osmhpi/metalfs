ARG METAL_TAG=
ARG VIVADO_EDITION=webpack
ARG PSL=PSL8

FROM metalfs/sdk-base:${METAL_TAG:+$METAL_TAG-}$VIVADO_EDITION AS build
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
 && make -j8 \
 && make install

FROM metalfs/sdk-base:${METAL_TAG:+$METAL_TAG-}$VIVADO_EDITION

COPY --from=build /usr/local/bin/metal-driver /usr/local/bin/

COPY --from=build /usr/local/include/ /usr/local/include/

COPY --from=build /usr/local/lib/libcxl.so /usr/local/lib/
COPY --from=build /usr/local/lib/libsnap.so /usr/local/lib/
COPY --from=build /usr/local/lib/libspdlog.so.1.5.0 /usr/local/lib/
RUN cd /usr/local/lib \
 && ln -s libspdlog.so.1.5.0 libspdlog.so.1 \
 && ln -s libspdlog.so.1 libspdlog.so

COPY --from=build /usr/local/lib/libmetal-*.so.1.0.0 /usr/local/lib/
RUN cd /usr/local/lib \
 && ln -s libmetal-driver-messages.so.1.0.0 libmetal-driver-messages.so.1 \
 && ln -s libmetal-driver-messages.so.1 libmetal-driver-messages.so
RUN cd /usr/local/lib \
 && ln -s libmetal-filesystem-pipeline.so.1.0.0 libmetal-filesystem-pipeline.so.1 \
 && ln -s libmetal-filesystem-pipeline.so.1 libmetal-filesystem-pipeline.so
RUN cd /usr/local/lib \
 && ln -s libmetal-filesystem.so.1.0.0 libmetal-filesystem.so.1 \
 && ln -s libmetal-filesystem.so.1 libmetal-filesystem.so
RUN cd /usr/local/lib \
 && ln -s libmetal-pipeline.so.1.0.0 libmetal-pipeline.so.1 \
 && ln -s libmetal-pipeline.so.1 libmetal-pipeline.so

COPY --from=build /usr/local/lib/cmake /usr/local/lib/

COPY --from=build /usr/local/share/metal_fs /usr/local/share/metal_fs
COPY --from=build /usr/local/share/snap /usr/local/share/snap
COPY --from=build /usr/local/share/cxl /usr/local/share/cxl

RUN ldconfig

ADD . /sdk/metal_fs
ENV METAL_ROOT=/sdk/metal_fs
