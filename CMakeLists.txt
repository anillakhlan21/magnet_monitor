cmake_minimum_required(VERSION 3.16.0)

project(Embridge_Modbus_MQTT_SQLite)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(tools /home/al034/Desktop/Workspace/SDKs/RUT2XX_R_GPL_00.01.14.6/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2)
set(CMAKE_C_COMPILER ${tools}/bin/mips-openwrt-linux-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/mips-openwrt-linux-g++)

set(Libs libjsoncpp.so.1.9.4 libmosquitto.so libsqlite3.so libmodbus.so libssl.so libcrypto.so)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# link_directories(usr/lib)
# link_directories(/usr/lib/x86_64-linux-gnu)

link_directories(/home/al034/Desktop/Workspace/SDKs/RUT2XX_R_GPL_00.01.14.6/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/lib)
link_directories(/home/al034/Desktop/Workspace/SDKs/RUT2XX_R_GPL_00.01.14.6/rutos-ath79-rut2-gpl/staging_dir/target-mips_34kc_uClibc-0.9.33.2/root-ar71xx/usr/lib)

add_executable(
    ${PROJECT_NAME}
    cpp/main.cpp
    cpp/settings.cpp
    cpp/mqtt.cpp
    cpp/modbus.cpp
    cpp/db.cpp
    )

include_directories(/usr/include)
include_directories(header)
include_directories(lib/json)
include_directories(/home/al034/Desktop/Workspace/SDKs/RUT2XX_R_GPL_00.01.14.6/rutos-ath79-rut2-gpl/staging_dir/target-mips_34kc_uClibc-0.9.33.2/usr/include)

target_link_libraries(${PROJECT_NAME} ${Libs})

