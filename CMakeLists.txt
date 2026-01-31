cmake_minimum_required(VERSION 3.10)
project(MagnetMonitor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
include_directories(${SRC_DIR})

set(COMMON_SOURCES
  ${SRC_DIR}/config.cpp
  ${SRC_DIR}/utils.cpp
  ${SRC_DIR}/ftp_downloader.cpp
  ${SRC_DIR}/parser.cpp
  ${SRC_DIR}/main.cpp
)

add_executable(magnet_monitor ${COMMON_SOURCES})

find_package(CURL REQUIRED)
if (TARGET CURL::libcurl)
  target_link_libraries(magnet_monitor PRIVATE CURL::libcurl)
else()
  target_link_libraries(magnet_monitor PRIVATE ${CURL_LIBRARIES})
endif()

# Try to find Paho MQTT C and C++ libraries (optional)
option(ENABLE_MQTT "Enable MQTT support (requires Paho C and C++ libraries)" ON)

if(ENABLE_MQTT)
  find_path(PAHO_INCLUDE_DIR mqtt/async_client.h)
  find_library(PAHO_CPP NAMES paho-mqttpp3 paho-mqttpp3_static)
  find_library(PAHO_C NAMES paho-mqtt3a paho-mqtt3a_static paho-mqtt3c paho-mqtt3c_static)

  if(PAHO_INCLUDE_DIR AND PAHO_CPP AND PAHO_C)
    message(STATUS "Found Paho MQTT: include=${PAHO_INCLUDE_DIR} libs=${PAHO_CPP} ${PAHO_C}")
    target_include_directories(magnet_monitor PRIVATE ${PAHO_INCLUDE_DIR})
    target_link_libraries(magnet_monitor PRIVATE ${PAHO_CPP} ${PAHO_C})
    # build with real MQTT publisher
    target_sources(magnet_monitor PRIVATE ${SRC_DIR}/mqtt_publisher.cpp)
  else()
    message(FATAL_ERROR "Paho MQTT C/C++ libraries or headers not found.\nPlease install them (macOS Homebrew):\n  brew install paho-mqtt-c paho-mqttpp3\nOr disable MQTT support: cmake -DENABLE_MQTT=OFF ..")
  endif()
else()
  message(STATUS "Building without MQTT support (ENABLE_MQTT=OFF). MQTT publish will be disabled at runtime.")
  # add a no-op stub so compilation succeeds when MQTT is disabled
  target_sources(magnet_monitor PRIVATE ${SRC_DIR}/mqtt_publisher_stub.cpp)
endif()

install(TARGETS magnet_monitor RUNTIME DESTINATION bin)
