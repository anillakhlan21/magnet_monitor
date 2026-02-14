#include <iostream>
#include <unistd.h>
#include <curl/curl.h>

#include "config.h"
#include "utils.h"
#include "ftp_downloader.h"
#include "parser.h"
#include "mqtt_publisher.h"
#include "memory_monitor.h"

struct CurlGlobalRAII {
    CurlGlobalRAII() { curl_global_init(CURL_GLOBAL_ALL); }
    ~CurlGlobalRAII() { curl_global_cleanup(); }
};

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

    // Log system time to verify dayXXYYZZ.dat calculation
    std::time_t boot_t = std::time(nullptr);
    char boot_time_str[64];
    std::strftime(boot_time_str, sizeof(boot_time_str), "%Y-%m-%d %H:%M:%S", std::localtime(&boot_t));
    write_log(cfg.log_file, "Application started. Gateway System Time: " + std::string(boot_time_str));
    write_log(cfg.log_file, "Note: If this time is wrong, your FTP filenames will be wrong (e.g. fetching day010170.dat).");

    // Login mechanism
    if (!cfg.app_username.empty() && !cfg.app_password.empty()) {
        std::string input_user, input_pass;
        std::cout << "--- Login Required ---" << std::endl;
        std::cout << "Username: ";
        if (!(std::cin >> input_user)) return 1;
        std::cout << "Password: ";
        if (!(std::cin >> input_pass)) return 1;

        if (input_user != cfg.app_username || input_pass != cfg.app_password) {
            std::cerr << "Login failed. Exiting." << std::endl;
            write_log(cfg.log_file, "Login failed for user: " + input_user);
            return 1;
        }
        write_log(cfg.log_file, "Login successful for user: " + input_user);
        std::cout << "Login successful." << std::endl;
    } else {
        write_log(cfg.log_file, "Started without login protection (no credentials in config).");
    }

    // Global initializations
    CurlGlobalRAII curl_raii;
    write_log(cfg.log_file,"curl_global_init");
    
    MQTTPublisher mqtt;
    write_log(cfg.log_file,"MQTT Init");
    
    // Initialize memory leak detector
    MemoryLeakDetector leak_detector;
    write_log(cfg.log_file, "Memory leak detector initialized at " + 
              MemoryMonitor::formatBytes(leak_detector.getBaselineMemory()));
    
    // Connect MQTT if in daemon mode (for run_once, we connect on demand)
    if (!run_once) {
        if (!mqtt.connect(cfg)) {
            std::cerr << "Initial MQTT connection failed (will retry in loop)." << std::endl;
            write_log(cfg.log_file, "Initial MQTT connection failed.");
        } else {
            write_log(cfg.log_file, "Initial MQTT connection established.");
        }
    }

    // Single run (useful for testing) -------------------------------------------------
    if (run_once) {
        std::string error;
        std::string remote_filename = discover_latest_file(cfg, error);
        
        bool success = false;
        if (!remote_filename.empty()) {
            write_log(cfg.log_file, "Single-run: Found latest file " + remote_filename);
            if (download_ftp(cfg, remote_filename, error)) {
                std::string latest_row = get_latest_row(cfg.local_file);
                success = mqtt.publish(cfg, latest_row);
            } else {
                std::cerr << "FTP download failed: " << error << std::endl;
                write_log(cfg.log_file, "Single-run: FTP download failed: " + error);
            }
        } else {
            std::cerr << "File discovery failed: " << error << std::endl;
            write_log(cfg.log_file, "Single-run: File discovery failed: " + error);
        }
        
        return success ? 0 : 1;
    }

    // Daemon mode: loop forever -------------------------------------------------------
    int cycle_count = 0;
    while (true) {
        try {
            cycle_count++;
            
            // Check for memory leaks every 10 cycles
            if (cycle_count % 10 == 0) {
                bool leak_found = leak_detector.checkAndLog(cfg.log_file, "Cycle " + std::to_string(cycle_count));
                if (leak_found) {
                    std::cerr << "⚠️  Memory leak detected! Check log file for details." << std::endl;
                }
            }
            
            std::string error;
            std::string remote_filename = discover_latest_file(cfg, error);
            
            if (!remote_filename.empty()) {
                write_log(cfg.log_file, "Cycle start: Latest file identified as " + remote_filename);
                if (download_ftp(cfg, remote_filename, error)) {
                    std::string latest_row = get_latest_row(cfg.local_file);
                    if (mqtt.publish(cfg, latest_row)) {
                        write_log(cfg.log_file, "Cycle success: Data published to MQTT.");
                    } else {
                        write_log(cfg.log_file, "Cycle warning: MQTT publish failed.");
                    }
                    sleep(cfg.poll_interval);
                } else {
                    std::cerr << "FTP download failed: " << error << std::endl;
                    write_log(cfg.log_file, "Cycle error: FTP failed: " + error);
                    std::cout << "Retrying FTP in " << cfg.retry_interval << " seconds..." << std::endl;
                    sleep(cfg.retry_interval);
                }
            } else {
                std::cerr << "File discovery failed: " << error << std::endl;
                write_log(cfg.log_file, "Cycle error: Discovery failed: " + error);
                sleep(cfg.retry_interval);
            }
        } catch (const std::exception& e) {
            std::string err_msg = "Unexpected error in monitor loop: " + std::string(e.what());
            std::cerr << err_msg << std::endl;
            write_log(cfg.log_file, err_msg);
            std::cout << "Restarting loop in 10 seconds..." << std::endl;
            sleep(10);
        } catch (...) {
            std::cerr << "Unknown error in monitor loop." << std::endl;
            write_log(cfg.log_file, "Unknown error in monitor loop.");
            sleep(10);
        }
    }

    return 0;
}
