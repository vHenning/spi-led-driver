#include <freertos/FreeRTOS.h>

#include "led_driver/LEDDriver.h"

extern "C" void app_main(void)
{
    LEDDriver driver(GPIO_NUM_4, 30);

    bool on = true;

    while (true)
    {
        driver.wait();
        driver.set(on ? 0xFFFFFFFFFF : 0);
        on = !on;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
