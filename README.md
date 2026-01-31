# Magnet Monitor — build & run

Small C++ service that downloads a daily data file from an FTP server, extracts the latest non-empty row, and publishes it to an MQTT broker (or prints the payload when built without MQTT).

Contents added
- Modularized code under `src/` (config, FTP downloader, parser, MQTT publisher, utilities, and `main.cpp`).
- `config.json` at project root with runtime settings.
- `CMakeLists.txt` to build the project.
- A no-op MQTT stub (`src/mqtt_publisher_stub.cpp`) so you can build and run without Paho MQTT libraries.

Prerequisites
- C++17-capable compiler (AppleClang/GCC/Clang)
- libcurl (FTP downloads). On macOS this is available via the SDK.
- Optional (for real MQTT publishing): Paho MQTT C and C++ libraries
  - macOS (Homebrew):
    ```bash
    brew install paho-mqtt-c paho-mqttpp3
    ```

Build
1. From the project root create a fresh build directory and configure with CMake.

Build WITHOUT real MQTT (safe for testing):

```bash
rm -rf build
mkdir build
cd build
cmake -DENABLE_MQTT=OFF ..
make -j2
```

Build WITH real MQTT (after installing Paho):

```bash
rm -rf build
mkdir build
cd build
cmake ..
make -j2
```

Notes about ENABLE_MQTT
- If `ENABLE_MQTT=ON` (default), CMake requires Paho headers and libraries and will error with an instruction to install them if they're missing.
- If `ENABLE_MQTT=OFF`, the build uses a stub `send_mqtt()` implementation that prints the payload instead of publishing.

Configuration (`config.json`)
- Located at project root. Edit this file to change runtime settings.
- Keys:
  - FTP: `FTP_HOST`, `FTP_USER`, `FTP_PASS`, `LOCAL_FILE`
  - MQTT: `MQTT_SERVER`, `MQTT_CLIENT_ID`, `MQTT_TOPIC`, `MQTT_USER`, `MQTT_PASS`
  - Intervals: `POLL_INTERVAL` (seconds), `RETRY_INTERVAL` (seconds)

How to run the application
- By default the program reads `config.json` from the current working directory. To avoid configuration errors, run the binary from the project root so it finds `config.json` automatically:

```bash
# from project root
./build/magnet_monitor
```

If you run the binary from the `build/` directory, the executable will look for `build/config.json` and likely fail to load configuration (this produces an exit with error). To run from `build/`, either copy or symlink the config file:

```bash
# run from build/ by copying the config file
cp ../config.json .
./magnet_monitor

# or create a symlink
ln -s ../config.json config.json
./magnet_monitor
```

Run in background and capture logs

```bash
# start in background and write stdout/stderr to magnet_monitor.log
nohup ./build/magnet_monitor > magnet_monitor.log 2>&1 &
echo $!   # prints background PID
tail -f magnet_monitor.log
```

Quick smoke test
- Create a sample data file (the `LOCAL_FILE` configured in `config.json`, default `/tmp/latest_data.dat`) to see the program read a row immediately:

```bash
echo "2026-01-31,42.0,OK" > /tmp/latest_data.dat
./build/magnet_monitor
# with MQTT disabled you'll see the printed payload in stdout/logs
```

Why you might see "exit code 1" when running the binary
- The most common cause is the executable cannot find `config.json` in the current working directory and exits with a configuration error. Run the binary from the repository root (or copy/symlink `config.json` into the working directory) to fix this.

Troubleshooting
- Missing Paho headers during CMake: install via Homebrew and re-run CMake
  ```bash
  brew install paho-mqtt-c paho-mqttpp3
  ```

- "No targets specified and no makefile found": ensure you ran `cmake ..` inside a fresh `build/` directory before running `make`.

- FTP connection errors: verify `FTP_HOST`, credentials, network accessibility, and firewall settings.

Optional system integration
- To run as a persistent macOS service use `launchd` (create a plist) or a process manager. Ask me and I can add a sample `launchd` plist.

Next steps I can help with
- Add a one-shot mode for testing (run a single download/publish cycle and exit).
- Add unit tests for `parser` and `config` (using GoogleTest or Catch2).
- Implement graceful shutdown (SIGINT/SIGTERM) so the service exits cleanly and can flush logs.

Run wrapper and macOS service
- `run.sh` — simple start/stop/status wrapper which rotates logs and writes a PID file. Usage:

```bash
# start
./run.sh start

# stop
./run.sh stop

# status
./run.sh status
```

- `com.invendis.magnetmonitor.plist` — example `launchd` plist (placed at project root). To install as a user service:

```bash
# copy plist to ~/Library/LaunchAgents and load it
cp com.invendis.magnetmonitor.plist ~/Library/LaunchAgents/
launchctl load ~/Library/LaunchAgents/com.invendis.magnetmonitor.plist

# To unload/remove later
launchctl unload ~/Library/LaunchAgents/com.invendis.magnetmonitor.plist
rm ~/Library/LaunchAgents/com.invendis.magnetmonitor.plist
```

Notes
- The plist uses absolute paths to the built executable and the project directory. Adjust them if you move the project.
- The plist is only an example; test it with `ENABLE_MQTT=OFF` first to ensure startup logs look healthy.

Cross-compiling for OpenWrt
---------------------------

If you have an OpenWrt SDK or toolchain (for example the SDK downloaded from the build system), you can cross-compile the project using the provided CMake toolchain file and helper script.

1) Prepare your OpenWrt toolchain
- Locate or install an OpenWrt SDK/toolchain for your target architecture. Common layouts expose a `bin/` directory containing cross-compilers and a `sysroot` or `staging_dir` containing target headers and libraries.

2) Configure the toolchain file
- Edit `toolchain-openwrt.cmake` at the project root and set the following variables near the top (or pass them to CMake via `-D`):
  - `OPENWRT_TOOLCHAIN_ROOT` — path to your OpenWrt SDK root (contains `bin/` and sysroot).
  - `OPENWRT_TARGET_TRIPLE` — the target triplet used by the toolchain (for example `arm-openwrt-linux-gnueabi`, `mipsel-openwrt-linux-gnu`, etc.).

3) Cross-build using the helper script
- A convenience script `scripts/cross_build.sh` is provided. Example usage:

```bash
# Build without MQTT (recommended for early testing):
./scripts/cross_build.sh /path/to/openwrt-sdk arm-openwrt-linux-gnueabi OFF

# Build with MQTT enabled (only if you cross-compiled/installed Paho into the SDK/sysroot):
./scripts/cross_build.sh /path/to/openwrt-sdk arm-openwrt-linux-gnueabi ON
```

The script creates a build directory named `build-openwrt-<target>` and runs CMake with `toolchain-openwrt.cmake`.

4) Notes on dependencies (libcurl / Paho)
- `libcurl` is typically available in the OpenWrt SDK/sysroot, but if it's not you must add it via OpenWrt feeds or compile it into the SDK.
- `Paho MQTT` libraries must be cross-built and installed into the SDK/sysroot if you want `ENABLE_MQTT=ON`. Two approaches:
  - Cross-compile Paho C and C++ libraries and install into the SDK/sysroot so the toolchain can find headers and libs.
  - Create an OpenWrt package (Makefile in the OpenWrt build system) for paho and this project; then use the SDK to build an `.ipk` package for the target.

5) Building an OpenWrt package (recommended for deployment)
- For production it's recommended to create an OpenWrt package so the service installs cleanly with opkg and declares runtime dependencies. If you want, I can scaffold a package Makefile that integrates with the OpenWrt build system and builds an IPK.

If you want me to run a cross-build here I can try, but I'll need either the exact OpenWrt SDK path you want me to use or the toolchain files from your attachment. Tell me which target triple and SDK path to use and I'll run the helper script and report results.

