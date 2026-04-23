FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y \
    wget xz-utils make git curl file \
    clang-format clang-tidy gdb-multiarch \
    && rm -rf /var/lib/apt/lists/*

RUN wget -q https://github.com/Pro/raspi-toolchain/releases/download/v1.0.1/raspi-toolchain.tar.gz \
    -O /tmp/rpi-toolchain.tar.gz \
    && tar -xf /tmp/rpi-toolchain.tar.gz -C /opt \
    && rm /tmp/rpi-toolchain.tar.gz

ENV PATH="/opt/cross-pi-gcc/bin:$PATH"
ENV SYSROOT="/opt/cross-pi-gcc/arm-linux-gnueabihf/sysroot"

# Activation de l'architecture armhf pour apt
RUN dpkg --add-architecture armhf && apt-get update

# Téléchargement et extraction des libs FFmpeg armhf dans le sysroot
RUN cd /tmp && apt-get download \
      libavcodec-dev:armhf \
      libavformat-dev:armhf \
      libavutil-dev:armhf \
      libswscale-dev:armhf \
      libavcodec59:armhf \
      libavformat59:armhf \
      libavutil57:armhf \
      libswscale6:armhf \
    && for deb in /tmp/*.deb; do \
         dpkg-deb -x "$deb" "$SYSROOT"; \
       done \
    && rm /tmp/*.deb \
    && cp -r $SYSROOT/usr/include/arm-linux-gnueabihf/* $SYSROOT/usr/include/ \
    && cp -r $SYSROOT/usr/lib/arm-linux-gnueabihf/* $SYSROOT/usr/lib/ \
    && rm -rf /var/lib/apt/lists/*