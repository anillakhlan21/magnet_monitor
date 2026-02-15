#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <string>
#include <memory>
#include <atomic>
#include "config.h"

struct mosquitto;

class MQTTPublisher {
public:
    MQTTPublisher();
    ~MQTTPublisher();

    bool connect(const Config& cfg);
    bool publish(const Config& cfg, const std::string& payload);
    void disconnect();

private:
    struct MosqDeleter {
        void operator()(struct mosquitto* m) const;
    };
    std::unique_ptr<struct mosquitto, MosqDeleter> mosq;
    bool connected;
    
    // Message delivery tracking
    std::atomic<int> last_mid;
    std::atomic<bool> message_delivered;
    
    // Mosquitto callbacks
    static void on_publish_callback(struct mosquitto* mosq, void* userdata, int mid);
};

#endif
