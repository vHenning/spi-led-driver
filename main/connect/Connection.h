#ifndef CONNECTION_H
#define CONNECTION_H

#include <esp_wifi.h>

class Connection
{
public:
    Connection(const char* ssid, const char* password);

    bool on;

private:
    static void wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData);

    static void udpTask(void* args);
};

#endif