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

    if (day_files.empty()) {
        error_out = "No valid 'day' files found in " + url;
        return "";
    }

    // Prefer numeric comparison of the index after "day" and before ".dat".
    // This handles names like day1, day2, day10 correctly even when not zero-padded.
    auto extract_index = [](const std::string& s) -> long long {
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        size_t pos = lower.find("day");
        if (pos == std::string::npos) return -1;
        pos += 3; // move past 'day'
        size_t dot = lower.rfind(".dat");
        if (dot == std::string::npos || dot <= pos) return -1;
        std::string numpart = lower.substr(pos, dot - pos);
        std::string digits;
        for (char c : numpart) if (std::isdigit((unsigned char)c)) digits.push_back(c);
        if (digits.empty()) return -1;
        try { return std::stoll(digits); } catch (...) { return -1; }
    };

    std::sort(day_files.begin(), day_files.end(), [&](const std::string& a, const std::string& b) {
        long long ia = extract_index(a);
        long long ib = extract_index(b);
        if (ia >= 0 && ib >= 0) return ia < ib; // numeric compare
        if (ia >= 0) return true;              // numeric sorts before non-numeric
        if (ib >= 0) return false;
        // fallback: case-insensitive lexical
        std::string la = a, lb = b;
        std::transform(la.begin(), la.end(), la.begin(), ::tolower);
        std::transform(lb.begin(), lb.end(), lb.begin(), ::tolower);
        return la < lb;
    });

    std::string latest = day_files.back();
    
    // Return full path if needed, or just filename. Python returns just filename and then appends it to path in RETR.
    // Our download_ftp expects the filename to be appended to cfg.ftp_host.
    // However, the directory path is /CFDisk/mindata/
    return "/CFDisk/mindata/" + latest;
}
