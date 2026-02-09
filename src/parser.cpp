#include "parser.h"
#include <fstream>
#include <string>

std::string get_latest_row(const std::string& local_file) {
    if (local_file.empty()) return "";
    
    try {
        std::ifstream file(local_file);
        if (!file.is_open()) {
            return "";
        }

        std::string line, last_line;
        while (std::getline(file, line)) {
            if (!line.empty()) last_line = line;
        }
        file.close();
        return last_line;
    } catch (const std::exception& e) {
        std::cerr << "Exception while parsing file: " << e.what() << std::endl;
        return "";
    }
}
