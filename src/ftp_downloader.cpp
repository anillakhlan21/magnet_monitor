#include "ftp_downloader.h"
#include "utils.h"
#include <curl/curl.h>
#include <cstdio>
#include <iostream>

#include <memory>

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

    CURLcode res = curl_easy_perform(curl.get());
    
    // Explicitly close file before renaming/removing
    fp.reset();

    if (res == CURLE_OK) {
        write_log(cfg.log_file, "curl_easy_perform Success");
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
