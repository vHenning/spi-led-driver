#include "MQTTProtocol.h"

#include <nvs_flash.h>
#include <esp_log.h>
#include <string.h>

#include "../libs/nlohmann/json.hpp"
using json = nlohmann::json;

MQTTProtocol::MQTTProtocol(std::string ssid, std::string key, std::string serverIP, std::string hostname)
    : client(0)
    , serverIP(serverIP)
    , wifiConnected(false)
    , mqttConnected(false)
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

    esp_netif_set_hostname(netIF, hostname.c_str());

    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&config);

    esp_event_handler_instance_t anyHandler;
    esp_event_handler_instance_t gotIPHandler;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &MQTTProtocol::wifiEventHandler, this, &anyHandler);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &MQTTProtocol::wifiEventHandler, this, &gotIPHandler);

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
    strncpy((char*) wifiConfig.sta.ssid, ssid.c_str(), sizeof(wifiConfig.sta.ssid));
    strncpy((char*) wifiConfig.sta.password, key.c_str(), sizeof(wifiConfig.sta.password));
    #pragma GCC diagnostic pop

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
    esp_wifi_start();
}

void MQTTProtocol::wifiEventHandler(void* arg, esp_event_base_t eventBase, int32_t eventID, void* eventData)
{
    MQTTProtocol* instance = static_cast<MQTTProtocol*>(arg);
    if (eventBase == WIFI_EVENT)
    {
        if (eventID == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        else if (eventID == WIFI_EVENT_STA_DISCONNECTED)
        {// Retry
            instance->wifiConnected = false;
            esp_wifi_connect();
        }
    }
    else if (eventBase == IP_EVENT && eventID == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(eventData);
        char buffer[50];
        esp_ip4addr_ntoa(&event->ip_info.ip, buffer, 50);
        instance->wifiConnected = true;
        ESP_LOGI("Connection", "Got IP: %s", buffer);

        instance->setupMQTT();
    }
}

void MQTTProtocol::addController(std::string name, CarLight* controller)
{
    controllers.insert(std::make_pair(name, controller));
    listen();
}

void MQTTProtocol::setupMQTT()
{
    if (client)
    {
        if (wifiConnected && !mqttConnected)
        {
            esp_mqtt_client_reconnect(client);
        }
        return;
    }

    std::string uri = "mqtt://" + serverIP;
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    esp_mqtt_client_config_t mqttConfig =
    {
        .broker = {.address = uri.c_str()}
    };
    #pragma GCC diagnostic pop

    client = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, MQTTProtocol::mqttEventHandler, this);
    esp_mqtt_client_start(client);
}

void MQTTProtocol::mqttEventHandler(void* handlerArgs, esp_event_base_t base, int32_t eventID, void* eventData)
{
    static const char* tag = "MQTTProtocol";
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(eventData);
    esp_mqtt_client_handle_t client = event->client;

    MQTTProtocol* instance = static_cast<MQTTProtocol*>(handlerArgs);

    switch (static_cast<esp_mqtt_event_id_t>(eventID))
    {
        case MQTT_EVENT_ERROR:
            break;
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(tag, "Connected, session present %d, wifi %s", event->session_present, instance->wifiConnected ? "connected" : "disconnected");
            instance->mqttConnected = true;
            instance->listen();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(tag, "Disconnected");
            instance->mqttConnected = false;
            if (instance->wifiConnected)
            {
                ESP_LOGI(tag, "Trying to reconnect");
                esp_mqtt_client_reconnect(client);
            }
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(tag, "published");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(tag, "Subscribed with response %.*s, error code %d", event->data_len, event->data, event->error_handle->error_type);
            break;
        case MQTT_EVENT_DATA:
            instance->handleMessage(std::string(event->topic, event->topic_len), std::string(event->data, event->data_len));
            break;
        default:
            ESP_LOGI(tag, "Unknown mqtt event, id %ld", eventID);
    }
}

void MQTTProtocol::listen()
{
    if (!mqttConnected)
    {
        return;
    }

    for (auto it = controllers.begin(); it != controllers.end(); ++it)
    {
        std::string topic = "LED/" + it->first + "/#";
        esp_mqtt_client_subscribe(client, topic.c_str(), 2);
    }
}

void MQTTProtocol::handleMessage(std::string topic, std::string message)
{
    // Topic is something like "LED/[device]/[command]"
    std::string deviceCommand = topic.substr(4);
    size_t slashIndex = deviceCommand.find_first_of("/");
    if (slashIndex == std::string::npos)
    {
        return;
    }
    std::string device = deviceCommand.substr(0, slashIndex);
    std::string command = deviceCommand.substr(slashIndex + 1);

    auto iterator = controllers.find(device);
    if (iterator == controllers.end())
    {
        return;
    }

    CarLight* controller = iterator->second;
    json msg = json::parse(message);

    char* tag = "MQTTProtocol";
    try
    {
        if (command.compare("color") == 0)
        {
            float red = static_cast<float>(msg["red"]) / 0xFF00;
            float green = static_cast<float>(msg["green"]) / 0xFF00;
            float blue = static_cast<float>(msg["blue"]) / 0xFF00;
            ESP_LOGI(tag, "Set %s color RGB %f %f %f", device.c_str(), red, green, blue);
            controller->setColor(red, green, blue);
        }
        else if (command.compare("colorDim") == 0)
        {
            float dim = msg["colorDim"];
            ESP_LOGI(tag, "Set %s color dim %f", device.c_str(), dim);
            controller->setColorBrightness(dim);
        }
        else if (command.compare("whiteDim") == 0)
        {
            float dim = msg["whiteDim"];
            ESP_LOGI(tag, "Set %s white dim %f", device.c_str(), dim);
            controller->setWhiteBrightness(dim);
        }
        else if (command.compare("whiteTemp") == 0)
        {
            float temp = msg["whiteTemp"];
            ESP_LOGI(tag, "Set %s white temp %f", device.c_str(), temp);
            controller->setWhiteTemperature(temp);
        }
        else if (command.compare("maxWhiteBrightness") == 0)
        {
            bool maxWhite = msg["maxWhiteBrightness"];
            ESP_LOGI(tag, "Set global max white brightness %s", maxWhite ? "on" : "off");
            ColorConverter::setMaxWhiteBrightness(maxWhite);
        }
        else if (command.compare("filterValues") == 0)
        {
            float capacitance = msg["capacitance"];
            float impedance = msg["impedance"];
            ESP_LOGI(tag, "Set %s filter values C=%f R=%f", device.c_str(), capacitance, impedance);
            controller->setFilterValues(capacitance, impedance);
        }
        else if (command.compare("filterValuesAndDelayed") == 0)
        {
            float capacitance = msg["capacitance"];
            float impedance = msg["impedance"];
            float x1 = msg["x1"];
            float y1 = msg["y1"];
            ESP_LOGI(tag, "Set %s filter values C=%f R=%f x1=%f y1=%f", device.c_str(), capacitance, impedance, x1, y1);
            controller->setFilterValues(capacitance, impedance);
            controller->setInitialFilterValues(x1, y1);
        }
        else if (command.compare("powerState") == 0)
        {
            bool on = msg["powerState"];
            ESP_LOGI(tag, "Turn %s %s", device.c_str(), on ? "on" : "off");
            on ? controller->turnOn() : controller->turnOff();
        }
        else
        {
            ESP_LOGI("MQTTProtocol", "Got unknown command (%s)", command.c_str());
        }
    }
    catch(const std::exception& e)
    {
        ESP_LOGI("MQTTProtocol", "Caught exception trying to read MQTT topic %s message %s", topic.c_str(), message.c_str());
    }
}
