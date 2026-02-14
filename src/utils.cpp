#include "utils.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fstream>


void write_log(const std::string& log_file, const std::string& message) {
    if (log_file.empty()) return;
    
    const long MAX_LOG_SIZE = 512 * 1024; // 512 KB

    // Check file size and rotate if necessary
    {
        std::ifstream ifs(log_file, std::ios::binary | std::ios::ate);
        if (ifs.is_open()) {
            long size = ifs.tellg();
            ifs.close(); // Always close before any file operations
            if (size > MAX_LOG_SIZE) {
                std::remove(log_file.c_str());
            }
        }
    }
    
    std::ofstream ofs(log_file, std::ios::app);
    if (ofs.is_open()) {
        std::time_t t = std::time(nullptr);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
        
        ofs << "[" << timestamp << "] " << message << std::endl;
        ofs.close();
    }
}
