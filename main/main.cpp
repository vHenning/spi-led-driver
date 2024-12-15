#include <freertos/FreeRTOS.h>

#include "animation/CarLight.h"

#include <esp_timer.h>
#include <esp_log.h>

const double FREQUENCY = 100; // Hz
const double PERIOD = 1 / FREQUENCY; // seconds
const int64_t PERIOD_MICROS = PERIOD * 1000 * 1000; // us
const int64_t PERIOD_MILLIS = PERIOD * 1000; // ms

extern "C" void app_main(void)
{
    CarLight light(4, PERIOD, 30, ColorConverter::rgb(1, 0, 0));

    bool on = true;
    int counter = 0;

    while (true)
    {
        int64_t startTime = esp_timer_get_time();

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

        int64_t diff = esp_timer_get_time() - startTime;
        int64_t microsToGo = PERIOD_MICROS - diff;
        int64_t millisToGo = microsToGo / 1000;

        if (millisToGo > 0)
        {
            vTaskDelay(millisToGo / portTICK_PERIOD_MS); // TODO Tick time to big (10ms) for this to be accurate
        }
    }
}
