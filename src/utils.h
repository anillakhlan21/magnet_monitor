#pragma once

#include <string>

// Returns remote filename in format: /CFDisk/mindata/dayDDMMYY.dat
std::string get_remote_filename();

// Appends message with timestamp to a log file
void write_log(const std::string& log_file, const std::string& message);
