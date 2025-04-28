#ifndef MQTTPROTOCOL_H
#define MQTTPROTOCOL_H

#include <string>
#include <map>

#include <esp_wifi.h>
#include <mqtt_client.h>

#include "../animation/CarLight.h"

class MQTTProtocol
{
public:
    MQTTProtocol(std::string ssid, std::string key, std::string serverIP, std::string hostname);

    void addController(std::string name, CarLight* controller);

private:
    static void wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData);
    static void mqttEventHandler(void* handlerArgs, esp_event_base_t base, int32_t eventID, void* eventData);

    void setupMQTT();
    void listen();

    void handleMessage(std::string topic, std::string message);

    std::map<std::string, CarLight*> controllers;

    esp_mqtt_client_handle_t client;
    std::string serverIP;

    bool wifiConnected;
    bool mqttConnected;
};

#endif // MQTTPROTOCOL_H
