#pragma once

#include <string>

struct Config {
    std::string ftp_host;
    std::string ftp_user;
    std::string ftp_pass;
    std::string local_file;

    std::string mqtt_server;
    std::string mqtt_client_id;
    std::string mqtt_topic;
    std::string mqtt_user;
    std::string mqtt_pass;

    int poll_interval{300};
    int retry_interval{120};

    std::string log_file;
    std::string app_username;
    std::string app_password;

    // Load configuration from a simple JSON file (flat key/value pairs only)
    bool load_from_file(const std::string& path);
};
