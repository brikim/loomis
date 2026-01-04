# --- Stage 1: Build Environment ---
FROM debian:trixie-slim AS build
LABEL maintainer="BK"

# Install build dependencies: g++, cmake, make, etc.
RUN echo "deb http://deb.debian.org/debian sid main" | tee -a /etc/apt/sources.list && \
	apt update && \
	apt upgrade -y && \
    apt install -y --no-install-recommends \
    build-essential \
	clang \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# Create the build directory
WORKDIR /app/build

# Copy source code into the container
COPY . /app/src

# Configure and build the project
ENV CXX=clang++
RUN cmake ../src -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -j 4

# --- Stage 2: Runtime Environment ---
FROM debian:trixie-slim AS runtime

ENV TZ=America/Chicago
ENV CONFIG_PATH='/config'
ENV LOG_PATH='/logs'

RUN apt update && \
	apt upgrade -y && \
	rm -rf /var/lib/apt/lists/*

# Copy only the compiled executable from the 'build' stage
COPY --from=build /app/build/loomis /usr/local/bin/loomis

# Command to run the application
CMD ["/usr/local/bin/loomis"]
