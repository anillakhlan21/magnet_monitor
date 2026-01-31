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

# Link against jsoncpp and libmosquitto (C library). libcurl will be located below.
set(Libs jsoncpp mosquitto pthread)

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

# --- Locate libcurl (prefer SDK staging include/lib if present) ------------------
set(CURL_INCLUDE_DIR "")
set(CURL_LIBRARY "")
if(DEFINED TOOLCHAIN_DIR)
  set(SDK_CURL_INC ${TOOLCHAIN_DIR}/target-mipsel_24kc_musl/usr/include)
  set(SDK_CURL_LIB ${TOOLCHAIN_DIR}/target-mipsel_24kc_musl/usr/lib)
  find_path(CURL_INCLUDE_DIR NAMES curl/curl.h PATHS ${SDK_CURL_INC} NO_DEFAULT_PATH)
  find_library(CURL_LIBRARY NAMES curl PATHS ${SDK_CURL_LIB} NO_DEFAULT_PATH)
endif()

# Fall back to system find if SDK copy wasn't found
if(NOT CURL_INCLUDE_DIR OR NOT CURL_LIBRARY)
  find_package(CURL QUIET)
  if(CURL_FOUND)
    if(NOT CURL_INCLUDE_DIR)
      set(CURL_INCLUDE_DIR ${CURL_INCLUDE_DIR})
    endif()
    if(NOT CURL_LIBRARY)
      set(CURL_LIBRARY ${CURL_LIBRARIES})
    endif()
  endif()
endif()

if(CURL_INCLUDE_DIR)
  target_include_directories(${PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIR})
  message(STATUS "Using curl headers: ${CURL_INCLUDE_DIR}")
else()
  message(WARNING "Could not find curl headers (curl/curl.h). FTP download will fail to compile unless you install libcurl-dev or point TOOLCHAIN_DIR to an SDK with curl.")
endif()

if(CURL_LIBRARY)
  list(APPEND Libs ${CURL_LIBRARY})
  message(STATUS "Linking curl library: ${CURL_LIBRARY}")
else()
  # keep the plain name as a fallback (may work if SDK linker paths are set)
  list(APPEND Libs curl)
  message(STATUS "Falling back to linking 'curl' by name; may still fail if library not available")
endif()

target_link_libraries(${PROJECT_NAME} ${Libs})

