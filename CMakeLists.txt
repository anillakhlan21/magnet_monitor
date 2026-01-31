cmake_minimum_required(VERSION 3.16.0)

project(magnetic_monitor)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(TOOLCHAIN_DIR /home/al034/Desktop/Workspace/SDKs/openwrt-sdk-ramips-mt76x8_gcc-7.5.0_musl.Linux-x86_64/staging_dir)
# allow overriding the toolchain path via -DTOOLCHAIN_DIR=...
if(DEFINED TOOLCHAIN_DIR)
  set(tools ${TOOLCHAIN_DIR}/toolchain-mipsel_24kc_gcc-7.5.0_musl)
endif()
set(CMAKE_C_COMPILER ${tools}/bin/mipsel-openwrt-linux-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/mipsel-openwrt-linux-g++)

# keep pthread flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# Link against jsoncpp, libmosquitto (C library) and libcurl (for FTP)
set(Libs jsoncpp mosquitto curl pthread)

# link directories from SDK
link_directories(${TOOLCHAIN_DIR}/target-mipsel_24kc_musl/usr/lib)
link_directories(${TOOLCHAIN_DIR}/target-mipsel_24kc_musl/root-ramips/usr/lib)

add_executable(
    ${PROJECT_NAME}
    src/main.cpp
    src/config.cpp
    src/ftp_downloader.cpp
  src/mqtt_publisher.cpp
    src/parser.cpp
    src/utils.cpp
)

# include paths (add SDK includes)
include_directories(/usr/include)
include_directories(header)
include_directories(lib/json)
include_directories(${TOOLCHAIN_DIR}/target-mipsel_24kc_musl/usr/include)

target_link_libraries(${PROJECT_NAME} ${Libs})

