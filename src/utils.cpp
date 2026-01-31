#include "utils.h"
#include <ctime>
#include <sstream>
#include <iomanip>

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
