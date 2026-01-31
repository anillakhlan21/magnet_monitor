#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <json/json.h>

bool Config::load_from_file(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "Could not open config file: " << path << std::endl;
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    Json::CharReaderBuilder builder;
    std::string errs;
    Json::Value root;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(content.c_str(), content.c_str() + content.size(), &root, &errs)) {
        std::cerr << "Failed to parse JSON: " << errs << std::endl;
        return false;
    }

    ftp_host = root.get("FTP_HOST", "").asString();
    ftp_user = root.get("FTP_USER", "").asString();
    ftp_pass = root.get("FTP_PASS", "").asString();
    local_file = root.get("LOCAL_FILE", "").asString();

    mqtt_server = root.get("MQTT_SERVER", "").asString();
    mqtt_client_id = root.get("MQTT_CLIENT_ID", "").asString();
    mqtt_topic = root.get("MQTT_TOPIC", "").asString();
    mqtt_user = root.get("MQTT_USER", "").asString();
    mqtt_pass = root.get("MQTT_PASS", "").asString();

    poll_interval = root.get("POLL_INTERVAL", poll_interval).asInt();
    retry_interval = root.get("RETRY_INTERVAL", retry_interval).asInt();

    // Basic validation
    if (ftp_host.empty() || ftp_user.empty() || ftp_pass.empty() || local_file.empty()) {
        std::cerr << "Incomplete FTP configuration in " << path << std::endl;
        return false;
    }
    if (mqtt_server.empty() || mqtt_topic.empty()) {
        std::cerr << "Incomplete MQTT configuration in " << path << std::endl;
        return false;
    }

    return true;
}
