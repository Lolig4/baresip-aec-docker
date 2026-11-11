# syntax=docker/dockerfile:1.7
ARG IMAGE=ghcr.io/baresip/docker/libre:latest
FROM ${IMAGE} AS build

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
      clang-19 ccache curl libopus-dev libasound2-dev libmosquitto-dev \
      libspandsp-dev libpulse-dev libwebrtc-audio-processing-dev

# The Debian development package ships public headers only. Add the matching
# upstream 1.3 headers so the module can configure the exported AEC3 classes.
RUN mkdir -p /usr/include/webrtc-audio-processing-1 && \
    curl -fsSL https://deb.debian.org/debian/pool/main/w/webrtc-audio-processing/webrtc-audio-processing_1.3.orig.tar.gz | \
      tar -xz --strip-components=2 -C /usr/include/webrtc-audio-processing-1 \
        webrtc-audio-processing-1.3/webrtc

# Baresip source: the root of this repository (build context).
COPY . /root/baresip/

WORKDIR /root/baresip
ENV CCACHE_DIR=/root/.cache/ccache
RUN --mount=type=cache,target=/root/.cache/ccache,sharing=locked \
    cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS="-g" \
      -DCMAKE_CXX_FLAGS="-g -DWEBRTC_APM_DEBUG_DUMP=0" \
      -DCMAKE_C_COMPILER=clang-19 \
      -DCMAKE_CXX_COMPILER=clang++-19 \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DCMAKE_INSTALL_PREFIX=/usr && \
    cmake --build build -j1 && \
    DESTDIR=/root/dist cmake --install build --prefix /usr && \
    cp -a /root/re/dist/. /root/dist/usr/ && \
    ccache -s

FROM debian:trixie-slim
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    apt-get update && apt-get install -y --no-install-recommends \
      libopus0 ca-certificates openssl libasound2 libmosquitto1 \
      libspandsp2 libpulse0 pulseaudio libwebrtc-audio-processing-1-3 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=build /root/dist/usr/ /usr/
WORKDIR /root
USER root
ENV USER=root
COPY docker/unraid-aec/entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
CMD ["baresip"]
