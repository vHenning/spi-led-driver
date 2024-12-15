#include <freertos/FreeRTOS.h>

#include "animation/CarLight.h"
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
    CarLight light(PERIOD, LED_COUNT, ColorConverter::rgb(1, 0, 0));
    LEDDriver driver(GPIO_NUM_4, LED_COUNT);

    bool on = true;
    int counter = 0;

    TickType_t previousWake = xTaskGetTickCount();

    while (true)
    {
        if (counter++ > 5 * FREQUENCY) // 5 seconds
        {
            counter = 0;
            on = !on;
            if (on)
            {
                light.turnOn();
            }
            else
            {
                light.turnOff();
            }
        }

        light.step();

        ColorConverter::rgb* colors = light.getPixels();

        for (int i = 0; i < LED_COUNT; ++i)
        {
            driver.set(i, ColorConverter::to8BitBRG(colors[i]));
        }

        driver.wait();
        driver.refresh();

        xTaskDelayUntil(&previousWake, pdMS_TO_TICKS(PERIOD_MILLIS));
    }
}
