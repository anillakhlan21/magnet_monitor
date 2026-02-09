#include "utils.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fstream>

std::string get_remote_filename() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    std::ostringstream oss;
    oss << "/CFDisk/mindata/day"
        << std::setfill('0') << std::setw(2) << now->tm_mday
        << std::setfill('0') << std::setw(2) << (now->tm_mon + 1)
        << std::setfill('0') << std::setw(2) << (now->tm_year % 100)
        << ".dat";
    return oss.str();
}

void write_log(const std::string& log_file, const std::string& message) {
    if (log_file.empty()) return;
    
    const long MAX_LOG_SIZE = 512 * 1024; // 512 KB

    // Check file size and rotate if necessary
    {
        std::ifstream ifs(log_file, std::ios::binary | std::ios::ate);
        if (ifs.is_open()) {
            if (ifs.tellg() > MAX_LOG_SIZE) {
                ifs.close();
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
