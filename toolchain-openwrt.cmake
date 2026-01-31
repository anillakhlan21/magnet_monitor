## OpenWrt CMake toolchain file
# Usage: edit the variables below to match your OpenWrt SDK/toolchain layout.
# Then configure CMake with: cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-openwrt.cmake <other-flags> ..

set(OPENWRT_TOOLCHAIN_ROOT "")
set(OPENWRT_TARGET_TRIPLE "")
set(OPENWRT_SYSROOT "")

if(OPENWRT_TOOLCHAIN_ROOT STREQUAL "" OR OPENWRT_TARGET_TRIPLE STREQUAL "")
  message(FATAL_ERROR "Please set OPENWRT_TOOLCHAIN_ROOT and OPENWRT_TARGET_TRIPLE at top of toolchain-openwrt.cmake or pass them via -DOPENWRT_TOOLCHAIN_ROOT=... -DOPENWRT_TARGET_TRIPLE=...")
endif()

if(OPENWRT_SYSROOT STREQUAL "")
  # Commonly the sysroot is under toolchain/staging_dir
  set(OPENWRT_SYSROOT "${OPENWRT_TOOLCHAIN_ROOT}/sysroot")
endif()

set(CMAKE_SYSTEM_NAME Linux)
# Set the target processor; adjust as needed (arm, armv7, mipsel, x86_64, etc.)
set(CMAKE_SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")

# Compilers
set(CMAKE_C_COMPILER   "${OPENWRT_TOOLCHAIN_ROOT}/bin/${OPENWRT_TARGET_TRIPLE}-gcc")
set(CMAKE_CXX_COMPILER "${OPENWRT_TOOLCHAIN_ROOT}/bin/${OPENWRT_TARGET_TRIPLE}-g++")

# Sysroot and search paths
set(CMAKE_SYSROOT "${OPENWRT_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH "${OPENWRT_SYSROOT};${OPENWRT_TOOLCHAIN_ROOT}")

# Search behavior: search for programs on host, libraries/includes on target
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Tell pkg-config (if present in toolchain) to use the sysroot
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${OPENWRT_SYSROOT}")
set(ENV{PKG_CONFIG_PATH} "${OPENWRT_SYSROOT}/usr/lib/pkgconfig;${OPENWRT_SYSROOT}/usr/share/pkgconfig")

# Optionally set more flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --sysroot=${CMAKE_SYSROOT}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --sysroot=${CMAKE_SYSROOT}")

message(STATUS "Using OpenWrt toolchain root: ${OPENWRT_TOOLCHAIN_ROOT}")
message(STATUS "Using OpenWrt target triple: ${OPENWRT_TARGET_TRIPLE}")
message(STATUS "Using OpenWrt sysroot: ${OPENWRT_SYSROOT}")
