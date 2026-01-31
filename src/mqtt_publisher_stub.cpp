#include "mqtt_publisher.h"
#include <iostream>

// No-op MQTT publisher used when MQTT support is disabled at build time.
void send_mqtt(const Config& cfg, const std::string& payload) {
    (void)cfg;
    if (payload.empty()) return;
    std::cout << "MQTT disabled at build time — payload not sent: " << payload << std::endl;
}
