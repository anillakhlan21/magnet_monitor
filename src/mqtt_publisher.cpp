#include "mqtt_publisher.h"
#include <mosquitto.h>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include "utils.h"

MQTTPublisher::MQTTPublisher() : connected(false), last_mid(0), message_delivered(false) {
    mosquitto_lib_init();
}

MQTTPublisher::~MQTTPublisher() {
    disconnect();
    mosquitto_lib_cleanup();
}

void MQTTPublisher::MosqDeleter::operator()(struct mosquitto* m) const {
    if (m) mosquitto_destroy(m);
}

// Callback when message is published successfully
void MQTTPublisher::on_publish_callback(struct mosquitto* mosq, void* userdata, int mid) {
    MQTTPublisher* publisher = static_cast<MQTTPublisher*>(userdata);
    if (publisher && mid == publisher->last_mid) {
        publisher->message_delivered = true;
    }
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

        mosq.reset(mosquitto_new(cfg.mqtt_client_id.empty() ? nullptr : cfg.mqtt_client_id.c_str(), true, this));
        if (!mosq) {
            std::cerr << "Failed to create mosquitto instance" << std::endl;
            write_log(cfg.log_file, "Failed to create mosquitto instance");
            return false;
        }

        // Set callback for publish confirmation
        mosquitto_publish_callback_set(mosq.get(), on_publish_callback);

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

        // Start the network loop in a background thread
        int loop_rc = mosquitto_loop_start(mosq.get());
        if (loop_rc != MOSQ_ERR_SUCCESS) {
            std::string err_msg = "MQTT loop_start failed: " + std::string(mosquitto_strerror(loop_rc));
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

    // Reset delivery flag
    message_delivered = false;
    int mid = 0;

    int rc = mosquitto_publish(mosq.get(), &mid, cfg.mqtt_topic.c_str(), static_cast<int>(payload.size()), payload.data(), 1, false);
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

    // Store message ID for callback verification
    last_mid = mid;
    
    // Wait for message to be delivered (max 5 seconds)
    int wait_count = 0;
    const int max_wait = 50; // 50 * 100ms = 5 seconds
    while (!message_delivered && wait_count < max_wait) {
        usleep(100000); // 100ms
        wait_count++;
    }
    
    if (message_delivered) {
        std::cout << "Data sent to MQTT successfully (confirmed delivery)." << std::endl;
        write_log(cfg.log_file, "Data sent to MQTT successfully (confirmed delivery)");
        return true;
    } else {
        std::string warn_msg = "MQTT publish queued but delivery not confirmed within 5 seconds (mid=" + std::to_string(mid) + ")";
        std::cerr << "WARNING: " << warn_msg << std::endl;
        write_log(cfg.log_file, "WARNING: " + warn_msg);
        // Still return true since publish was queued successfully
        return true;
    }
}

void MQTTPublisher::disconnect() {
    if (mosq) {
        if (connected) {
            // Stop the background loop thread before disconnecting
            mosquitto_loop_stop(mosq.get(), false);
            mosquitto_disconnect(mosq.get());
        }
        mosq.reset();
    }
    connected = false;
}
