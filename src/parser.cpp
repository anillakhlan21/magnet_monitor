#include "parser.h"
#include <fstream>
#include <string>
#include <iostream>

std::string get_latest_row(const std::string& local_file) {
    if (local_file.empty()) return "";
    
    try {
        // Open in binary mode to handle \r and \n correctly regardless of platform
        std::ifstream file(local_file, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }

        std::string last_line;
        std::string current_line;
        char ch;
        
        // Read file character by character to avoid loading entire file into memory
        while (file.get(ch)) {
            if (ch == '\n') {
                // End of line - check if current_line has content
                if (current_line.find_first_not_of(" \t\r") != std::string::npos) {
                    last_line = current_line;
                }
                current_line.clear();
            } else if (ch == '\r') {
                // Handle \r - peek next char to see if it's \r\n
                if (file.peek() == '\n') {
                    file.get(ch); // consume the \n
                }
                // End of line
                if (current_line.find_first_not_of(" \t\r") != std::string::npos) {
                    last_line = current_line;
                }
                current_line.clear();
            } else {
                current_line += ch;
            }
        }
        
        // Don't forget the last line if file doesn't end with newline
        if (current_line.find_first_not_of(" \t\r\n") != std::string::npos) {
            last_line = current_line;
        }
        
        file.close();

        // Final trim of any whitespace around the data
        if (!last_line.empty()) {
            size_t first = last_line.find_first_not_of(" \t\r\n");
            size_t last = last_line.find_last_not_of(" \t\r\n");
            if (first != std::string::npos && last != std::string::npos) {
                last_line = last_line.substr(first, (last - first + 1));
            }
            
            std::cout << "Latest row valid. Length: " << last_line.length() << " characters." << std::endl;
            std::string snippet = last_line.length() > 60 ? last_line.substr(0, 60) + "..." : last_line;
            std::cout << "Data: [" << snippet << "]" << std::endl;
        }
        
        return last_line;
    } catch (const std::exception& e) {
        std::cerr << "Exception while parsing file: " << e.what() << std::endl;
        return "";
    }
}
