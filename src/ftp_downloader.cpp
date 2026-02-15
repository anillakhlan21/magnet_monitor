#include "ftp_downloader.h"
#include "utils.h"
#include <curl/curl.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <cctype>

#include <vector>
#include <algorithm>
#include <sstream>

static size_t list_callback(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!stream) return 0;
    return fwrite(ptr, size, nmemb, stream);
}

bool download_ftp(const Config& cfg, const std::string& remote_filename, std::string& error_out) {
    auto curl_deleter = [](CURL* c) { if (c) curl_easy_cleanup(c); };
    std::unique_ptr<CURL, decltype(curl_deleter)> curl(curl_easy_init(), curl_deleter);

    if (!curl) {
        error_out = "Failed to initialize curl";
        write_log(cfg.log_file, "curl_easy_init Failed");
        return false;
    }
    write_log(cfg.log_file, "curl_easy_init Success");

    bool success = false;
    std::string url = "ftp://" + cfg.ftp_host + remote_filename;
    std::string tmp_local = cfg.local_file + ".tmp";

    auto file_deleter = [](FILE* f) { if (f) fclose(f); };
    std::unique_ptr<FILE, decltype(file_deleter)> fp(fopen(tmp_local.c_str(), "wb"), file_deleter);

    if (!fp) {
        error_out = "Failed to open temp file for writing: " + tmp_local;
        write_log(cfg.log_file, error_out);
        return false;
    }
    
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_USERNAME, cfg.ftp_user.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, cfg.ftp_pass.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, fp.get());
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl.get(), CURLOPT_USE_SSL, CURLUSESSL_TRY);
    curl_easy_setopt(curl.get(), CURLOPT_MAXAGE_CONN, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_TCP_NODELAY, 0L);
    curl_easy_setopt(curl.get(), CURLOPT_FRESH_CONNECT, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_FORBID_REUSE, 1L);

    CURLcode res = curl_easy_perform(curl.get());
    
    // Explicitly close file before renaming/removing
    fp.reset();

    if (res == CURLE_OK) {
        double dl;
        curl_easy_getinfo(curl.get(), CURLINFO_SIZE_DOWNLOAD, &dl);
        write_log(cfg.log_file, "curl_easy_perform Success. Size: " + std::to_string((long)dl) + " bytes");
        
        if (std::rename(tmp_local.c_str(), cfg.local_file.c_str()) != 0) {
            error_out = "Failed to rename temp file to final path";
            write_log(cfg.log_file, error_out);
        } else {
            success = true;
        }
    } else {
        error_out = std::string("FTP Download Failed: ") + curl_easy_strerror(res);
        write_log(cfg.log_file, error_out);
        std::remove(tmp_local.c_str());
    }

    if (success) {
        write_log(cfg.log_file, "FTP: Successfully downloaded " + remote_filename);
    } // Errors already logged above
    
    return success;
}

std::string discover_latest_file(const Config& cfg, std::string& error_out) {
    auto curl_deleter = [](CURL* c) { if (c) curl_easy_cleanup(c); };
    std::unique_ptr<CURL, decltype(curl_deleter)> curl(curl_easy_init(), curl_deleter);

    if (!curl) {
        error_out = "Failed to initialize curl for discovery";
        return "";
    }

    std::string file_list;
    // The directory containing records
    std::string url = "ftp://" + cfg.ftp_host + "/CFDisk/mindata/";

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_USERNAME, cfg.ftp_user.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_PASSWORD, cfg.ftp_pass.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_DIRLISTONLY, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, list_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &file_list);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl.get());
    if (res != CURLE_OK) {
        error_out = "FTP List Failed: " + std::string(curl_easy_strerror(res));
        return "";
    }

    // Log the raw directory listing for triage
    write_log(cfg.log_file, std::string("FTP raw listing for /CFDisk/mindata/:\n") + file_list);

    std::vector<std::string> day_files;
    std::stringstream ss(file_list);
    std::string filename;
    while (std::getline(ss, filename)) {
        // Trim any trailing \r
        if (!filename.empty() && filename.back() == '\r') filename.pop_back();
        
        // Match dayXXXXXX.dat (case insensitive check like Python)
        std::string lower_f = filename;
        std::transform(lower_f.begin(), lower_f.end(), lower_f.begin(), ::tolower);
        
        if (lower_f.find("day") == 0 && lower_f.size() >= 12 && lower_f.substr(lower_f.size() - 4) == ".dat") {
            day_files.push_back(filename);
        }
    }

    // Log discovered candidates
    if (!day_files.empty()) {
        for (const auto& f : day_files) {
            write_log(cfg.log_file, std::string("Found candidate file: ") + f);
        }
    }

    if (day_files.empty()) {
        error_out = "No valid 'day' files found in " + url;
        return "";
    }

    // Prefer date-aware comparison. Filenames are expected to contain a date
    // like dayDDMMYY.dat or dayDDMMYYYY.dat. Parse day, month, year and compare
    // by (year, month, day) so 15 Feb 2026 (150226) > 31 Jan 2026 (310126).
    auto parse_date = [](const std::string& s) -> std::tuple<int,int,int> {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        size_t pos = lower.find("day");
        if (pos == std::string::npos) return std::tuple<int,int,int>{-1,-1,-1};
        pos += 3;
        size_t dot = lower.rfind(".dat");
        if (dot == std::string::npos || dot <= pos) return std::tuple<int,int,int>{-1,-1,-1};
        std::string numpart = lower.substr(pos, dot - pos);
        std::string digits;
        for (char c : numpart) if (std::isdigit((unsigned char)c)) digits.push_back(c);
        if (digits.size() == 6) {
            // DDMMYY
            int dd = std::stoi(digits.substr(0,2));
            int mm = std::stoi(digits.substr(2,2));
            int yy = std::stoi(digits.substr(4,2));
            int year = 2000 + yy; // assume 2000s
            return std::tuple<int,int,int>{year, mm, dd};
        } else if (digits.size() == 8) {
            // DDMMYYYY
            int dd = std::stoi(digits.substr(0,2));
            int mm = std::stoi(digits.substr(2,2));
            int year = std::stoi(digits.substr(4,4));
            return std::tuple<int,int,int>{year, mm, dd};
        }
        return std::tuple<int,int,int>{-1,-1,-1};
    };

    std::sort(day_files.begin(), day_files.end(), [&](const std::string& a, const std::string& b) {
        auto da = parse_date(a);
        auto db = parse_date(b);
        if (std::get<0>(da) >= 0 && std::get<0>(db) >= 0) {
            if (da != db) return da < db; // tuple comparison on (year,month,day)
            return a < b;
        }
        if (std::get<0>(da) >= 0) return true;  // valid date sorts before invalid
        if (std::get<0>(db) >= 0) return false;
        // fallback: case-insensitive lexical
        std::string la = a, lb = b;
        std::transform(la.begin(), la.end(), la.begin(), ::tolower);
        std::transform(lb.begin(), lb.end(), lb.begin(), ::tolower);
        return la < lb;
    });

    std::string latest = day_files.back();

    // Log the sorted list with parsed dates for debugging
    for (const auto& f : day_files) {
        auto d = parse_date(f);
        if (std::get<0>(d) >= 0) {
            write_log(cfg.log_file, std::string("Sorted candidate: ") + f + std::string("  date=") +
                      std::to_string(std::get<0>(d)) + "-" + std::to_string(std::get<1>(d)) + "-" + std::to_string(std::get<2>(d)));
        } else {
            write_log(cfg.log_file, std::string("Sorted candidate: ") + f + std::string("  date=(na)"));
        }
    }

    write_log(cfg.log_file, std::string("Selected latest file: ") + latest);
    
    // Return full path if needed, or just filename. Python returns just filename and then appends it to path in RETR.
    // Our download_ftp expects the filename to be appended to cfg.ftp_host.
    // However, the directory path is /CFDisk/mindata/
    return "/CFDisk/mindata/" + latest;
}
