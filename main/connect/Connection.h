#ifndef CONNECTION_H
#define CONNECTION_H

#include <esp_wifi.h>
#include <functional>

class Connection
{
public:
    Connection(const char* ssid, const char* password);

    std::function<void (const uint8_t*, size_t)> packetHandler;

private:
    static void wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData);

    static void udpTask(void* args);
};

#endif