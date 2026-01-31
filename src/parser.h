#pragma once

#include <string>

// Returns the last non-empty line from the given local file. Returns empty string if not available.
std::string get_latest_row(const std::string& local_file);
