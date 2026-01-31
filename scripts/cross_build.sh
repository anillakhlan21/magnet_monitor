#!/usr/bin/env bash
set -euo pipefail

# Simple wrapper to cross-compile the project with an OpenWrt toolchain file.
# Usage:
#   ./scripts/cross_build.sh /path/to/openwrt/toolchain target-triple [--enable-mqtt]
# Example:
#   ./scripts/cross_build.sh /opt/openwrt-sdk arm-openwrt-linux-gnueabi

TOOLCHAIN_ROOT=${1:-}
TARGET_TRIPLE=${2:-}
ENABLE_MQTT_FLAG=${3:-"OFF"}

if [ -z "$TOOLCHAIN_ROOT" ] || [ -z "$TARGET_TRIPLE" ]; then
  echo "Usage: $0 <OPENWRT_TOOLCHAIN_ROOT> <TARGET_TRIPLE> [ON|OFF for ENABLE_MQTT]"
  exit 2
fi

BUILD_DIR="build-openwrt-${TARGET_TRIPLE}"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR"

cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../toolchain-openwrt.cmake \
  -DOPENWRT_TOOLCHAIN_ROOT="$TOOLCHAIN_ROOT" \
  -DOPENWRT_TARGET_TRIPLE="$TARGET_TRIPLE" \
  -DENABLE_MQTT=$ENABLE_MQTT_FLAG

make -j$(nproc || echo 1)

popd

echo "Cross-build complete. Binary (if produced) is in $BUILD_DIR/" 
