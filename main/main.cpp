#include <freertos/FreeRTOS.h>

#include "animation/CarLight.h"

#include <esp_timer.h>
#include <esp_log.h>

// We are limited by the RTOS tick frequency which is 100 Hz by default on the ESP32.
// Choose the frequency so the period is a multiple of one tick because we cannot delay for fractions of a tick.
const double FREQUENCY = 50; // [Hz]
const double PERIOD = 1 / FREQUENCY; // seconds
const int64_t PERIOD_MILLIS = PERIOD * 1000; // ms

extern "C" void app_main(void)
{
    CarLight light(4, PERIOD, 20, ColorConverter::rgb(1, 0, 0));

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

        xTaskDelayUntil(&previousWake, pdMS_TO_TICKS(PERIOD_MILLIS));
    }
}
