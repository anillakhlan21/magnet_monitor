#pragma once

#include <string>
#include "config.h"

// Download the remote file from FTP to the configured local file (atomic rename on success)
// Returns true on success, false on failure. On failure an optional message can be set in error_out.
bool download_ftp(const Config& cfg, const std::string& remote_filename, std::string& error_out);

// Find the correct dayDDMMYY.dat file using FTP server time (not local time)
// This ensures correct file selection even when device time is wrong
std::string discover_latest_file(const Config& cfg, std::string& error_out);
