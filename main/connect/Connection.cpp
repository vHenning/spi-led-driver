#include "Connection.h"

#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_netif.h>

#include <lwip/sockets.h>

Connection::Connection(const char* ssid, const char* password, const char* ip)
{
    // NVS is used for SSID and password storage
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();

    esp_event_loop_create_default();
    esp_netif_t* netIF = esp_netif_create_default_wifi_sta();
    esp_netif_ip_info_t ipInfo;
    ipInfo.ip.addr = esp_ip4addr_aton(ip);
    ipInfo.gw.addr = esp_ip4addr_aton("192.168.0.1");
    ipInfo.netmask.addr = esp_ip4addr_aton("255.255.255.0");

    esp_netif_dhcpc_stop(netIF);
    esp_netif_set_ip_info(netIF, &ipInfo);

    esp_netif_set_hostname(netIF, "led_desk");

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);

    esp_event_handler_instance_t anyHandler;
    esp_event_handler_instance_t gotIPHandler;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &Connection::wifiEventHandler, this, &anyHandler);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &Connection::wifiEventHandler, this, &gotIPHandler);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    wifi_config_t wifiConfig =
    {
        .sta =
        {
            .ssid = "",
            .password = ""
        }
    };
    strncpy((char*) wifiConfig.sta.ssid, ssid, sizeof(wifiConfig.sta.ssid));
    strncpy((char*) wifiConfig.sta.password, password, sizeof(wifiConfig.sta.password));
    #pragma GCC diagnostic pop

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    esp_wifi_start();
}

void Connection::wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData)
{
    if (eventBase == WIFI_EVENT)
    {
        if (eventID == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        else if (eventID == WIFI_EVENT_STA_DISCONNECTED)
        {// Retry
            esp_wifi_connect();
        }
    }
    else if (eventBase == IP_EVENT && eventID == IP_EVENT_STA_GOT_IP)
    {
        Connection* instance = static_cast<Connection*>(arg);

        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(eventData);
        char buffer[50];
        esp_ip4addr_ntoa(&event->ip_info.ip, buffer, 50);
        ESP_LOGI("Connection", "Got IP: %s", buffer);

        xTaskCreate(&Connection::udpTask, "udpTask", 4096, instance, 5, NULL);
    }
}

void Connection::udpTask(void* args)
{
    Connection* instance = static_cast<Connection*>(args);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 1)
    {
        ESP_LOGI("Connection", "Error creating socket %d", errno);
    }
    else
    {
        ESP_LOGI("Connection", "Created socket");
    }
    sockaddr_in myAddress;
    myAddress.sin_family = AF_INET;
    myAddress.sin_addr.s_addr = INADDR_ANY;
    myAddress.sin_port = htons(8002);
    bind(sock, (sockaddr*) &myAddress, sizeof(myAddress));

    const size_t BUFFER_SIZE = 512;
    uint8_t buffer[BUFFER_SIZE];

    sockaddr_in fromAddress;
    socklen_t fromLength;

    while (true)
    {
        size_t received = recvfrom(sock, buffer, BUFFER_SIZE, 0, (sockaddr*) &fromAddress, &fromLength);
        instance->packetHandler(buffer, received);
    }
}
