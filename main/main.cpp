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

#define LED_COUNT 20

extern "C" void app_main(void)
{
    LEDDriver driver(GPIO_NUM_4, LED_COUNT);
    ColorConverter::hsvcct color(ColorConverter::hsv(0, 0, 0), 4000, 1);
    CarLight light(PERIOD, LED_COUNT, ColorConverter::hsv2rgb(color));
    Connection conn(WIFI_SSID, WIFI_PASSWORD);
    LEDProtocol ledProtocol(&light);

    conn.packetHandler = std::bind(&LEDProtocol::parse, &ledProtocol, std::placeholders::_1, std::placeholders::_2);

    TickType_t previousWake = xTaskGetTickCount();

    while (true)
    {
        light.step();
        ColorConverter::rgbcct* colors = light.getPixels();

        for (int i = 0; i < LED_COUNT; ++i)
        {
            driver.set(i, ColorConverter::to8BitWWBRG(colors[i]));
        }

        driver.wait();
        driver.refresh();

        xTaskDelayUntil(&previousWake, pdMS_TO_TICKS(PERIOD_MILLIS));
    }
}
