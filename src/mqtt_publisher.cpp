#include "mqtt_publisher.h"
#include <mosquitto.h>
#include <iostream>
#include <algorithm>

// Simple synchronous publish using libmosquitto (C API).
void send_mqtt(const Config& cfg, const std::string& payload) {
    if (payload.empty()) return;

    // Initialize library
    mosquitto_lib_init();

    // Parse server into host and optional port
    std::string host = cfg.mqtt_server;
    int port = 1883;
    // strip common URL schemes
    if (host.rfind("tcp://", 0) == 0) host = host.substr(6);
    else if (host.rfind("mqtt://", 0) == 0) host = host.substr(7);

    auto pos = host.rfind(':');
    if (pos != std::string::npos) {
        std::string port_str = host.substr(pos + 1);
        if (!port_str.empty() && std::all_of(port_str.begin(), port_str.end(), ::isdigit)) {
            try { port = std::stoi(port_str); host = host.substr(0, pos); }
            catch (...) { /* leave defaults */ }
        }
    }

    struct mosquitto *mosq = mosquitto_new(cfg.mqtt_client_id.empty() ? nullptr : cfg.mqtt_client_id.c_str(), true, nullptr);
    if (!mosq) {
        std::cerr << "Failed to create mosquitto instance" << std::endl;
        mosquitto_lib_cleanup();
        return;
    }

    if (!cfg.mqtt_user.empty()) {
        mosquitto_username_pw_set(mosq, cfg.mqtt_user.c_str(), cfg.mqtt_pass.c_str());
    }

    int rc = mosquitto_connect(mosq, host.empty() ? nullptr : host.c_str(), port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "MQTT connect failed: " << mosquitto_strerror(rc) << std::endl;
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return;
    }

    // Publish (synchronous: mosquitto_loop_start could be used for async)
    rc = mosquitto_publish(mosq, nullptr, cfg.mqtt_topic.c_str(), static_cast<int>(payload.size()), payload.data(), 1, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::cerr << "MQTT publish failed: " << mosquitto_strerror(rc) << std::endl;
    } else {
        std::cout << "Data sent to cloud successfully." << std::endl;
    }

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}
