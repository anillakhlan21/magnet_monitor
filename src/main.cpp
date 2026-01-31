#include <iostream>
#include <unistd.h>

#include "config.h"
#include "utils.h"
#include "ftp_downloader.h"
#include "parser.h"
#include "mqtt_publisher.h"

int main(int argc, char* argv[]) {
    std::cout << "Starting C++ Magnet Monitor Service..." << std::endl;

    bool run_once = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--once" || a == "-1") run_once = true;
        if (a == "--help" || a == "-h") {
            std::cout << "Usage: magnet_monitor [--once]\n"
                      << "  --once   Run one download/parse/publish cycle and exit" << std::endl;
            return 0;
        }
    }

    Config cfg;
    const std::string config_path = "config.json";
    if (!cfg.load_from_file(config_path)) {
        std::cerr << "Failed to load configuration. Exiting." << std::endl;
        return 1;
    }

    // Single run (useful for testing) -------------------------------------------------
    if (run_once) {
        std::string remote_filename = get_remote_filename();
        std::string error;
        if (download_ftp(cfg, remote_filename, error)) {
            std::string latest_row = get_latest_row(cfg.local_file);
            send_mqtt(cfg, latest_row);
            return 0;
        } else {
            std::cerr << "FTP download failed: " << error << std::endl;
            return 1;
        }
    }

    // Daemon mode: loop forever -------------------------------------------------------
    while (true) {
        std::string remote_filename = get_remote_filename();
        std::string error;
        if (download_ftp(cfg, remote_filename, error)) {
            std::string latest_row = get_latest_row(cfg.local_file);
            send_mqtt(cfg, latest_row);
            sleep(cfg.poll_interval);
        } else {
            std::cerr << "FTP download failed: " << error << std::endl;
            std::cout << "Retrying FTP in " << cfg.retry_interval << " seconds..." << std::endl;
            sleep(cfg.retry_interval);
        }
    }

    return 0;
}
