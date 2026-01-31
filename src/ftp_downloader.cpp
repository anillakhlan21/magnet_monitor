#include "ftp_downloader.h"
#include <curl/curl.h>
#include <cstdio>
#include <iostream>

static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    if (!stream) return 0;
    return fwrite(ptr, size, nmemb, stream);
}

bool download_ftp(const Config& cfg, const std::string& remote_filename, std::string& error_out) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        error_out = "Failed to initialize curl";
        return false;
    }

    bool success = false;
    std::string url = "ftp://" + cfg.ftp_host + remote_filename;
    std::string tmp_local = cfg.local_file + ".tmp";

    FILE* fp = fopen(tmp_local.c_str(), "wb");
    if (!fp) {
        error_out = "Failed to open temp file for writing: " + tmp_local;
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERNAME, cfg.ftp_user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, cfg.ftp_pass.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_TRY);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);

    if (res == CURLE_OK) {
        if (std::rename(tmp_local.c_str(), cfg.local_file.c_str()) != 0) {
            error_out = "Failed to rename temp file to final path";
        } else {
            success = true;
        }
    } else {
        error_out = std::string("FTP Download Failed: ") + curl_easy_strerror(res);
        // remove tmp file on failure
        std::remove(tmp_local.c_str());
    }

    curl_easy_cleanup(curl);
    return success;
}
