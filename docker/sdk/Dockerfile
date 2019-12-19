ARG VIVADO_EDITION=webpack
ARG PSL=psl8

FROM metalfs/sdk-base:$VIVADO_EDITION-$PSL AS build

ADD . /src/metal_fs
RUN cd /src/metal_fs \
 && mkdir build \
 && cd build \
 && cmake \
    -DCMAKE_INSTALL_PREFIX=/install/ \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPTION_BUILD_TESTS=OFF .. \
 && make && make install

FROM metalfs/sdk-base:$VIVADO_EDITION-$PSL
COPY --from=build /install/metal-driver /usr/local/bin/
COPY --from=build /install/metal-driver-placeholder /usr/local/bin/
COPY --from=build /install/lib/ /usr/local/lib/

RUN ldconfig
