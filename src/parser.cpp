#include "parser.h"
#include <fstream>
#include <string>

std::string get_latest_row(const std::string& local_file) {
    std::ifstream file(local_file);
    std::string line, last_line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (!line.empty()) last_line = line;
        }
        file.close();
    }
    return last_line;
}
