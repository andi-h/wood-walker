FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    dkms \
    device-tree-compiler \
    bison \
    flex \
    shim-signed \
    zstd \
    && apt-get clean

WORKDIR /usr/src/rcio

COPY Makefile ./Makefile
COPY dkms.conf ./dkms.conf
COPY rcio-overlay.dts ./rcio-overlay.dts
COPY src ./src
COPY install.sh ./install.sh

RUN chmod +x ./install.sh
ENTRYPOINT ["./install.sh"]

