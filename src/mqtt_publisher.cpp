#include "mqtt_publisher.h"
#include <mqtt/async_client.h>
#include <iostream>

void send_mqtt(const Config& cfg, const std::string& payload) {
    if (payload.empty()) return;

    mqtt::async_client client(cfg.mqtt_server, cfg.mqtt_client_id);
    mqtt::connect_options connOpts;
    connOpts.set_user_name(cfg.mqtt_user);
    connOpts.set_password(cfg.mqtt_pass);
    connOpts.set_clean_session(true);

    try {
        client.connect(connOpts)->wait();
        client.publish(cfg.mqtt_topic, payload, 1, false)->wait();
        client.disconnect()->wait();
        std::cout << "Data sent to cloud successfully." << std::endl;
    } catch (const mqtt::exception& exc) {
        std::cerr << "MQTT Error: " << exc.what() << std::endl;
    }
}
