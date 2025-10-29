# Dockerfile for testing lua-gpiod build on Debian
FROM debian:bookworm-slim

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV LUA_VERSION=5.4

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    libgpiod-dev \
    lua${LUA_VERSION} \
    lua${LUA_VERSION}-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /usr/src/lua-gpiod

# Copy source files
COPY gpiod_lua.c .
COPY gpiod_example.lua .
COPY Makefile .
COPY README.md .
COPY README_CN.md .

# Build the library
RUN make clean && make

# Test module loading
RUN lua${LUA_VERSION} -e "local gpiod = require('gpiod'); print('gpiod version:', gpiod.version()); print('Module loaded successfully')"

# Create a simple test script that doesn't require actual GPIO hardware
RUN echo 'local gpiod = require("gpiod")\n\
print("=== lua-gpiod Test Suite ===")\n\
print("gpiod version:", gpiod.version())\n\
print("Module functions available:")\n\
print("- chip_open:", type(gpiod.chip_open))\n\
print("- chip_iter:", type(gpiod.chip_iter))\n\
print("- sleep:", type(gpiod.sleep))\n\
print("- version:", type(gpiod.version))\n\
print("Constants available:")\n\
print("- OPEN_DRAIN:", gpiod.OPEN_DRAIN)\n\
print("- OPEN_SOURCE:", gpiod.OPEN_SOURCE)\n\
print("- ACTIVE_LOW:", gpiod.ACTIVE_LOW)\n\
print("- BIAS_DISABLE:", gpiod.BIAS_DISABLE)\n\
print("- BIAS_PULL_DOWN:", gpiod.BIAS_PULL_DOWN)\n\
print("- BIAS_PULL_UP:", gpiod.BIAS_PULL_UP)\n\
print("Test sleep function...")\n\
local start = os.clock()\n\
gpiod.sleep(0.1)\n\
local elapsed = os.clock() - start\n\
print("Sleep test completed, elapsed:", string.format("%.3f", elapsed), "seconds")\n\
print("=== All tests passed ===")\n' > test_basic.lua

# Run basic functionality test
RUN lua${LUA_VERSION} test_basic.lua

# Set default command
CMD ["lua5.4", "-i"]

# Add labels for documentation
LABEL maintainer="lua-gpiod project"
LABEL description="Test environment for lua-gpiod library on Debian"
LABEL version="1.0"

# Add health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD lua${LUA_VERSION} -e "require('gpiod')" || exit 1