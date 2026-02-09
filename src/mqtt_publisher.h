#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <string>
#include <memory>
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
};

#endif
