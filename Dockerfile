# --- Stage 1: Build & Chisel ---
FROM debian:trixie-slim AS build
LABEL maintainer="BK"

# 1. Install standard build tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git ninja-build ca-certificates libssl-dev curl \
    && rm -rf /var/lib/apt/lists/*

# 2. Install 'chisel' (the tool that creates the tiny runtime)
RUN curl -sSL https://github.com/canonical/chisel/releases/download/v1.0.0/chisel_v1.0.0_linux_amd64.tar.gz | tar -xz -C /usr/local/bin

WORKDIR /app
COPY . .

# 3. Build Loomis
RUN cmake -G Ninja -B build -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON && \
    cmake --build build --config Release --parallel 4

# 4. Create a tiny root filesystem for the runtime
# We take only the essential slices: base-files, libc, and libstdc++
RUN mkdir /rootfs && \
    chisel cut --release ubuntu-24.04 --root /rootfs \
    base-files_base \
    base-files_release-info \
    ca-certificates_data \
    libc6_libs \
    libgcc-s1_libs \
    libstdc++6_libs \
    libssl3_libs \
    tzdata_zoneinfo

# Copy the binary into our new rootfs
COPY --from=build /app/build/loomis /rootfs/usr/local/bin/loomis
RUN chmod +x /rootfs/usr/local/bin/loomis

# --- Stage 2: Runtime Environment ---
# We start from 'scratch' and copy in the sculpted rootfs
FROM scratch AS runtime

ENV TZ=America/Chicago
ENV CONFIG_PATH='/config'
ENV LOG_PATH='/logs'

# Copy the entire sculpted filesystem
COPY --from=build /rootfs /

ENTRYPOINT ["/usr/local/bin/loomis"]
