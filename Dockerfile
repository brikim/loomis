# --- Stage 1: Build & Chisel ---
FROM debian:trixie-slim AS build
LABEL maintainer="BK"

# 1. Install build tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git ninja-build ca-certificates libssl-dev curl tar \
    && rm -rf /var/lib/apt/lists/*

# 2. Install 'chisel' tool
RUN curl -sSL https://github.com/canonical/chisel/releases/download/v1.0.0/chisel_v1.0.0_linux_amd64.tar.gz | tar -xz -C /usr/local/bin

WORKDIR /app
COPY . .

# 3. Build Loomis
RUN cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON && \
    cmake --build build --config Release --parallel 4

# 4. Create the tiny rootfs 
# NOTE: libssl3 changed to libssl3t64 in Ubuntu 24.04
RUN mkdir -p /rootfs && \
    chisel cut --release ubuntu-24.04 --root /rootfs \
    base-files_base \
    base-files_release-info \
    ca-certificates_data \
    libc6_libs \
    libgcc-s1_libs \
    libstdc++6_libs \
    libssl3t64_libs \
    openssl_config \
    tzdata_zoneinfo

# 5. Copy the binary manually within the build stage
RUN mkdir -p /rootfs/usr/local/bin && \
    cp /app/build/loomis /rootfs/usr/local/bin/loomis && \
    chmod +x /rootfs/usr/local/bin/loomis

# --- Stage 2: Runtime Environment ---
FROM scratch AS runtime

ENV TZ=America/Chicago
ENV CONFIG_PATH='/config'
ENV LOG_PATH='/logs'

# Copy the entire sculpted filesystem from the builder
COPY --from=build /rootfs /

ENTRYPOINT ["/usr/local/bin/loomis"]