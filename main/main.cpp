#include <freertos/FreeRTOS.h>

#include "WifiCredentials.h"

#include "animation/colors/GammaCorrection.h"
#include "animation/CarLight.h"
#include "connect/Connection.h"
#include "connect/LEDProtocol.h"
#include "connect/MQTTProtocol.h"
#include "led_driver/LEDDriver.h"

#include <esp_timer.h>
#include <esp_log.h>

#include <vector>

// We are limited by the RTOS tick frequency which is 100 Hz by default on the ESP32.
// Choose the frequency so the period is a multiple of one tick because we cannot delay for fractions of a tick.
const double FREQUENCY = 50; // [Hz]
const double PERIOD = 1 / FREQUENCY; // seconds
const int64_t PERIOD_MILLIS = PERIOD * 1000; // ms

const size_t DRIVER_COUNT = 2;

const int ledCounts[DRIVER_COUNT] = {
    // 64 // Living room
    31 // Dining room
  , 19 // Living room
};

const gpio_num_t pins[DRIVER_COUNT] = {
    GPIO_NUM_11
    , GPIO_NUM_12
};

const char* names[DRIVER_COUNT] = {
    "livingRoom"
    , "livingRoomAbove"
    // "diningRoom"
    // , "livingRoomTV"
};

extern "C" void app_main(void)
{
    LEDDriver** drivers = new LEDDriver*[DRIVER_COUNT];
    CarLight** lights = new CarLight*[DRIVER_COUNT];
    ColorConverter::hsvcct color(ColorConverter::hsv(0, 0, 0), 4000, 1);

    std::vector<int64_t*> previous;
    int skipCounter[DRIVER_COUNT];
    MQTTProtocol mqtt(WIFI_SSID, WIFI_PASSWORD, "192.168.0.80", "LED_Living_room");
    for (size_t i = 0; i < DRIVER_COUNT; ++i)
    {
        drivers[i] = new LEDDriver(pins[i], ledCounts[i]);
        lights[i] = new CarLight(PERIOD, ledCounts[i], ColorConverter::hsv2rgb(color));
        previous.push_back(new int64_t[ledCounts[i]]);
        for (int j = 0; j < ledCounts[i]; ++j)
        {
            previous[i][j] = 0;
        }
        mqtt.addController(names[i], lights[i]);
        lights[i]->turnOn();
        skipCounter[i] = 0;
    }

    TickType_t previousWake = xTaskGetTickCount();


    while (true)
    {
        for (size_t i = 0; i < DRIVER_COUNT; ++i)
        {
            lights[i]->step();
            ColorConverter::rgbcct* colors = lights[i]->getPixels();

            bool changed = false;
            for (size_t j = 0; j < ledCounts[i]; ++j)
            {
                uint64_t converted = ColorConverter::to8BitWWBRG(colors[j]);
                if (converted != previous[i][j])
                {
                    changed = true;
                }

                previous[i][j] = converted;
                drivers[i]->set(j, converted);
            }
            if (changed || skipCounter[i] > FREQUENCY)
            {
                drivers[i]->refresh();
                drivers[i]->wait();
                skipCounter[i] = 0;
            }
            else
            {
                skipCounter[i]++;
            }
        }

        xTaskDelayUntil(&previousWake, pdMS_TO_TICKS(PERIOD_MILLIS));
    }
}
