#pragma once

#include <string>
#include "config.h"

// Publishes payload to MQTT using configuration values. If payload is empty nothing is sent.
void send_mqtt(const Config& cfg, const std::string& payload);
