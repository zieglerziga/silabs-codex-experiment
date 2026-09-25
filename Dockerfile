# syntax=docker/dockerfile:1@sha256:ecfaec9ed6d810b56388c508f4121597bfbba70d41a6dfeee4d8cad5f295fc32

FROM buildpack-deps:noble-curl@sha256:20153d0e51926e10ed2e2c17343edae0080e4537fa6d613f784b6f64d2df0b69 AS certificates

FROM ubuntu:24.04@sha256:008173c23f95b170204355c12626cb5a965d779a7e1283b09e9cffbb1bf33ca3

ARG DEBIAN_FRONTEND=noninteractive
ARG UBUNTU_SNAPSHOT=20260925T090000Z

COPY --from=certificates /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt

RUN apt-get -o APT::Snapshot="$UBUNTU_SNAPSHOT" update \
    && apt-get -o APT::Snapshot="$UBUNTU_SNAPSHOT" install --yes --no-install-recommends \
        build-essential \
        ca-certificates \
        clang-format \
        cmake \
        gcc-arm-none-eabi \
        git \
        libnewlib-arm-none-eabi \
        make \
        ninja-build \
        python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
