#include <freertos/FreeRTOS.h>

#include "WifiCredentials.h"

#include "animation/colors/GammaCorrection.h"
#include "animation/CarLight.h"
#include "connect/Connection.h"
#include "connect/LEDProtocol.h"
#include "led_driver/LEDDriver.h"

#include <esp_timer.h>
#include <esp_log.h>

// We are limited by the RTOS tick frequency which is 100 Hz by default on the ESP32.
// Choose the frequency so the period is a multiple of one tick because we cannot delay for fractions of a tick.
const double FREQUENCY = 50; // [Hz]
const double PERIOD = 1 / FREQUENCY; // seconds
const int64_t PERIOD_MILLIS = PERIOD * 1000; // ms

#define TOTAL_COUNT 18
#define LEFT_COUNT 12
#define RIGHT_COUNT 6

extern "C" void app_main(void)
{
    LEDDriver leftDriver(GPIO_NUM_32, LEFT_COUNT);
    LEDDriver rightDriver(GPIO_NUM_33, RIGHT_COUNT);
    ColorConverter::hsvcct color(ColorConverter::hsv(0, 0, 0), 4000, 1);
    CarLight light(PERIOD, TOTAL_COUNT, ColorConverter::hsv2rgb(color));
    Connection conn(WIFI_SSID, WIFI_PASSWORD, "192.168.0.84");
    LEDProtocol ledProtocol(&light);

    conn.packetHandler = std::bind(&LEDProtocol::parse, &ledProtocol, std::placeholders::_1, std::placeholders::_2);

    TickType_t previousWake = xTaskGetTickCount();

    while (true)
    {
        light.step();
        ColorConverter::rgbcct* colors = light.getPixels();

        for (int i = 0; i < LEFT_COUNT; ++i)
        {
            leftDriver.set(i, ColorConverter::to8BitWWBRG(colors[i]));
        }
        for (int i = 0; i < RIGHT_COUNT; ++i)
        {
            rightDriver.set(i, ColorConverter::to8BitWWBRG(colors[i + LEFT_COUNT]));
        }

        leftDriver.wait();
        leftDriver.refresh();
        rightDriver.wait();
        rightDriver.refresh();

        xTaskDelayUntil(&previousWake, pdMS_TO_TICKS(PERIOD_MILLIS));
    }
}
