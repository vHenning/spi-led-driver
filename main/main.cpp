#include <freertos/FreeRTOS.h>

#include "animation/colors/GammaCorrection.h"
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
    LEDDriver driver(GPIO_NUM_4, LED_COUNT);

    double temperature = ColorConverter::WARM_TEMPERATURE;
    const double temperatureChange = 1000; // Kelvin per second

    TickType_t previousWake = xTaskGetTickCount();

    while (true)
    {
        temperature += temperatureChange * PERIOD;
        if (temperature >= ColorConverter::COLD_TEMPERATURE)
        {
            temperature = ColorConverter::WARM_TEMPERATURE;
        }

        ColorConverter::hsvcct color;
        color.color.v = 0;
        color.whiteValue = 0.7;
        color.whiteTemp = temperature;

        ColorConverter::rgbcct converted = ColorConverter::hsv2rgb(color);

        uint8_t warm = converted.ww * 0xFF;
        uint8_t cold = converted.cw * 0xFF;

        warm = gamma8[warm];
        cold = gamma8[cold];

        driver.set(0, 0, 0, warm, cold);

        xTaskDelayUntil(&previousWake, pdMS_TO_TICKS(PERIOD_MILLIS));
    }
}
