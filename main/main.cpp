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

// We are limited by the RTOS tick frequency which is 100 Hz by default on the ESP32.
// Choose the frequency so the period is a multiple of one tick because we cannot delay for fractions of a tick.
const double FREQUENCY = 50; // [Hz]
const double PERIOD = 1 / FREQUENCY; // seconds
const int64_t PERIOD_MILLIS = PERIOD * 1000; // ms

const char* hostname = "LED_kitchenSink";

const int LEFT_LED_COUNT = 12;
const int RIGHT_LED_COUNT = 6;

const gpio_num_t LEFT_PIN = GPIO_NUM_32;
const gpio_num_t RIGHT_PIN = GPIO_NUM_33;

extern "C" void app_main(void)
{
    ColorConverter::hsvcct color(ColorConverter::hsv(0, 0, 0), 4000, 1);

    int64_t* previous = new int64_t[LEFT_LED_COUNT + RIGHT_LED_COUNT];
    int leftSkipCounter = 0;
    int rightSkipCounter = 0;
    MQTTProtocol mqtt(WIFI_SSID, WIFI_PASSWORD, "192.168.0.80", hostname);

    LEDDriver leftDriver = LEDDriver(LEFT_PIN, LEFT_LED_COUNT);
    LEDDriver rightDriver = LEDDriver(RIGHT_PIN, RIGHT_LED_COUNT);
    CarLight light = CarLight(PERIOD, LEFT_LED_COUNT + RIGHT_LED_COUNT, ColorConverter::hsv2rgb(color));
    for (int i = 0; i < LEFT_LED_COUNT + RIGHT_LED_COUNT; ++i) {
        previous[i] = 0;
    }
    mqtt.addController("kitchenSink", &light);
    light.turnOn();

    TickType_t previousWake = xTaskGetTickCount();


    while (true)
    {
        light.step();
        ColorConverter::rgbcct* colors = light.getPixels();

        bool leftChanged = false;
        bool rightChanged = false;
        for (size_t i = 0; i < LEFT_LED_COUNT; ++i)
        {
            uint64_t converted = ColorConverter::to8BitWWBRG(colors[i]);
            if (converted != previous[i])
            {
                leftChanged = true;
            }

            previous[i] = converted;
            leftDriver.set(i, converted);
        }
        for (size_t i = 0; i < RIGHT_LED_COUNT; ++i) {
            uint64_t converted = ColorConverter::to8BitWWBRG(colors[i + LEFT_LED_COUNT]);
            if (converted != previous[i + LEFT_LED_COUNT]) {
                rightChanged = true;
            }
            previous[i + LEFT_LED_COUNT] = converted;
            rightDriver.set(i, converted);
        }
        if (leftChanged) {
            leftDriver.refresh();
            leftDriver.wait();
            leftSkipCounter = 0;
        }
        else {
            leftSkipCounter++;
        }
        if (rightChanged) {
            rightDriver.refresh();
            rightDriver.wait();
            rightSkipCounter = 0;
        }
        else {
            rightSkipCounter++;
        }

        if (leftSkipCounter > FREQUENCY || rightSkipCounter > FREQUENCY) {
            leftDriver.refresh();
            rightDriver.refresh();
            leftDriver.wait();
            rightDriver.wait();
            leftSkipCounter = 0;
            rightSkipCounter = 0;
        }

        xTaskDelayUntil(&previousWake, pdMS_TO_TICKS(PERIOD_MILLIS));
    }
}
