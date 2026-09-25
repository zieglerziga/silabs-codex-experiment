# syntax=docker/dockerfile:1@sha256:ecfaec9ed6d810b56388c508f4121597bfbba70d41a6dfeee4d8cad5f295fc32

FROM buildpack-deps:noble-curl@sha256:20153d0e51926e10ed2e2c17343edae0080e4537fa6d613f784b6f64d2df0b69 AS toolchain

ARG TARGETARCH
ARG ARM_GNU_VERSION=12.2.rel1
ARG UBUNTU_SNAPSHOT=20260925T090000Z

RUN sed -i \
      -e "s|http://archive.ubuntu.com/ubuntu/|https://snapshot.ubuntu.com/ubuntu/${UBUNTU_SNAPSHOT}/|" \
      -e "s|http://security.ubuntu.com/ubuntu/|https://snapshot.ubuntu.com/ubuntu/${UBUNTU_SNAPSHOT}/|" \
      /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get update \
    && apt-get install --yes --no-install-recommends xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN case "$TARGETARCH" in \
      amd64) \
        toolchain_host=x86_64; \
        toolchain_sha256=84be93d0f9e96a15addd490b6e237f588c641c8afdf90e7610a628007fc96867 \
        ;; \
      arm64) \
        toolchain_host=aarch64; \
        toolchain_sha256=7ee332f7558a984e239e768a13aed86c6c3ac85c90b91d27f4ed38d7ec6b3e8c \
        ;; \
      *) \
        echo "unsupported container architecture: $TARGETARCH" >&2; \
        exit 1 \
        ;; \
    esac \
    && archive="arm-gnu-toolchain-${ARM_GNU_VERSION}-${toolchain_host}-arm-none-eabi.tar.xz" \
    && curl --fail --location --retry 3 \
      "https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_GNU_VERSION}/binrel/${archive}" \
      --output "/tmp/${archive}" \
    && echo "${toolchain_sha256}  /tmp/${archive}" | sha256sum --check --strict \
    && mkdir -p /opt/arm-gnu-toolchain \
    && tar --extract --xz --file="/tmp/${archive}" \
      --directory=/opt/arm-gnu-toolchain --strip-components=1 \
    && rm "/tmp/${archive}"

FROM ubuntu:24.04@sha256:008173c23f95b170204355c12626cb5a965d779a7e1283b09e9cffbb1bf33ca3

ARG DEBIAN_FRONTEND=noninteractive
ARG UBUNTU_SNAPSHOT=20260925T090000Z

COPY --from=toolchain /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt
COPY --from=toolchain /opt/arm-gnu-toolchain /opt/arm-gnu-toolchain

ENV PATH="/opt/arm-gnu-toolchain/bin:${PATH}"

RUN sed -i \
      -e "s|http://archive.ubuntu.com/ubuntu/|https://snapshot.ubuntu.com/ubuntu/${UBUNTU_SNAPSHOT}/|" \
      -e "s|http://security.ubuntu.com/ubuntu/|https://snapshot.ubuntu.com/ubuntu/${UBUNTU_SNAPSHOT}/|" \
      /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get update \
    && apt-get install --yes --no-install-recommends \
        build-essential \
        ca-certificates \
        clang-format \
        cmake \
        git \
        make \
        ninja-build \
        python3 \
    && rm -rf /var/lib/apt/lists/* \
    && test "$(arm-none-eabi-gcc -dumpfullversion)" = 12.2.1

WORKDIR /workspace
