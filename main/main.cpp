#include <freertos/FreeRTOS.h>

#include "led_driver/LEDDriver.h"

extern "C" void app_main(void)
{
    LEDDriver driver(GPIO_NUM_4, 3);

    bool on = true;

    while (true)
    {
        driver.wait();
        uint8_t value = on ? 0xFF : 0;
        driver.set(0, 0, 0, value, value);
        on = !on;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
