#include "mqtt_publisher.h"
#include <mosquitto.h>
#include <iostream>
#include <algorithm>
#include "utils.h"

MQTTPublisher::MQTTPublisher() : connected(false) {
    mosquitto_lib_init();
}

MQTTPublisher::~MQTTPublisher() {
    disconnect();
    mosquitto_lib_cleanup();
}

void MQTTPublisher::MosqDeleter::operator()(struct mosquitto* m) const {
    if (m) mosquitto_destroy(m);
}

bool MQTTPublisher::connect(const Config& cfg) {
    if (connected && mosq) return true;

    // Ensure we clean up any old instance before recreating
    disconnect();

    try {
        // Parse server into host and optional port
        std::string host = cfg.mqtt_server;
        int port = 1883;
        if (host.rfind("tcp://", 0) == 0) host = host.substr(6);
        else if (host.rfind("mqtt://", 0) == 0) host = host.substr(7);

        auto pos = host.rfind(':');
        if (pos != std::string::npos) {
            std::string port_str = host.substr(pos + 1);
            if (!port_str.empty() && std::all_of(port_str.begin(), port_str.end(), ::isdigit)) {
                try { port = std::stoi(port_str); host = host.substr(0, pos); }
                catch (...) {}
            }
        }

        mosq.reset(mosquitto_new(cfg.mqtt_client_id.empty() ? nullptr : cfg.mqtt_client_id.c_str(), true, nullptr));
        if (!mosq) {
            std::cerr << "Failed to create mosquitto instance" << std::endl;
            write_log(cfg.log_file, "Failed to create mosquitto instance");
            return false;
        }

        if (!cfg.mqtt_user.empty()) {
            mosquitto_username_pw_set(mosq.get(), cfg.mqtt_user.c_str(), cfg.mqtt_pass.c_str());
        }

        int rc = mosquitto_connect(mosq.get(), host.empty() ? nullptr : host.c_str(), port, 60);
        if (rc != MOSQ_ERR_SUCCESS) {
            std::string err_msg = "MQTT connect failed: " + std::string(mosquitto_strerror(rc));
            std::cerr << err_msg << std::endl;
            write_log(cfg.log_file, err_msg);
            mosq.reset();
            return false;
        }

        connected = true;
        write_log(cfg.log_file, "MQTT: Connected to " + host + ":" + std::to_string(port));
    } catch (const std::exception& e) {
        std::cerr << "Exception in MQTT connect: " << e.what() << std::endl;
        write_log(cfg.log_file, "MQTT Exception: " + std::string(e.what()));
        mosq.reset();
        return false;
    }
    return true;
}

bool MQTTPublisher::publish(const Config& cfg, const std::string& payload) {
    if (payload.empty()) return true;
    if (!connected || !mosq) {
        if (!connect(cfg)) return false;
    }

    int rc = mosquitto_publish(mosq.get(), nullptr, cfg.mqtt_topic.c_str(), static_cast<int>(payload.size()), payload.data(), 1, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        std::string err_msg = "MQTT publish failed: " + std::string(mosquitto_strerror(rc));
        std::cerr << err_msg << std::endl;
        write_log(cfg.log_file, err_msg);
        // If connection is lost, mark it as disconnected so we retry next time
        if (rc == MOSQ_ERR_NO_CONN || rc == MOSQ_ERR_CONN_LOST) {
            connected = false;
        }
        return false;
    }

    std::cout << "Data sent to MQTT successfully." << std::endl;
    write_log(cfg.log_file, "Data sent to MQTT successfully");
    return true;
}

void MQTTPublisher::disconnect() {
    if (mosq) {
        if (connected) mosquitto_disconnect(mosq.get());
        mosq.reset();
    }
    connected = false;
}
