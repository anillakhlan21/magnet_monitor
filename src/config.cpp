#include "config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

// Very small, tolerant JSON extractor for flat key/value pairs used by this project.
static std::string extract_string(const std::string& json, const std::string& key) {
    std::string needle = '"' + key + '"';
    auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos);
    if (pos == std::string::npos) return {};
    pos++;
    // skip whitespace
    while (pos < json.size() && isspace((unsigned char)json[pos])) ++pos;
    if (pos >= json.size()) return {};
    if (json[pos] == '"') {
        ++pos;
        std::ostringstream oss;
        while (pos < json.size() && json[pos] != '"') {
            oss << json[pos++];
        }
        return oss.str();
    }
    return {};
}

static int extract_int(const std::string& json, const std::string& key, int fallback) {
    std::string needle = '"' + key + '"';
    auto pos = json.find(needle);
    if (pos == std::string::npos) return fallback;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return fallback;
    pos++;
    while (pos < json.size() && isspace((unsigned char)json[pos])) ++pos;
    if (pos >= json.size()) return fallback;
    std::string num;
    while (pos < json.size() && (isdigit((unsigned char)json[pos]) || json[pos] == '-')) {
        num.push_back(json[pos++]);
    }
    if (num.empty()) return fallback;
    return std::stoi(num);
}

bool Config::load_from_file(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "Could not open config file: " << path << std::endl;
        return false;
    }
    std::ostringstream ss;
    ss << ifs.rdbuf();
    std::string json = ss.str();

    ftp_host = extract_string(json, "FTP_HOST");
    ftp_user = extract_string(json, "FTP_USER");
    ftp_pass = extract_string(json, "FTP_PASS");
    local_file = extract_string(json, "LOCAL_FILE");

    mqtt_server = extract_string(json, "MQTT_SERVER");
    mqtt_client_id = extract_string(json, "MQTT_CLIENT_ID");
    mqtt_topic = extract_string(json, "MQTT_TOPIC");
    mqtt_user = extract_string(json, "MQTT_USER");
    mqtt_pass = extract_string(json, "MQTT_PASS");

    poll_interval = extract_int(json, "POLL_INTERVAL", poll_interval);
    retry_interval = extract_int(json, "RETRY_INTERVAL", retry_interval);

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
