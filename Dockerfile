# --- Stage 1: Build Environment ---
FROM debian:trixie-slim AS build
LABEL maintainer="BK"

RUN apt-get update && apt-get upgrade -y && \
    apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ninja-build \
    ca-certificates \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# We build as usual. Chiseled is Ubuntu-based (glibc), 
# so the Debian-compiled binary is perfectly compatible.
RUN cmake -G Ninja -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON && \
    cmake --build build --config Release --parallel 4

# IMPORTANT: Set permissions here, as the runtime image has no 'chmod'
RUN chmod +x /app/build/loomis

# --- Stage 2: Runtime Environment ---
FROM ubuntu/chiseled:24.04 AS runtime

# Application environment
ENV TZ=America/Chicago
ENV CONFIG_PATH='/config'
ENV LOG_PATH='/logs'

# Copy CA certs and TZ data from the build stage or standard paths
# Chiseled images often need these copied in if the app does HTTPS
COPY --from=build /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/
COPY --from=build /usr/share/zoneinfo/America/Chicago /usr/share/zoneinfo/America/Chicago

# Copy the binary
COPY --from=build /app/build/loomis /usr/local/bin/loomis

# Since there is no shell, we use the Exec form for CMD
ENTRYPOINT ["/usr/local/bin/loomis"]
