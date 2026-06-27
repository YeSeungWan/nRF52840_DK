#include <zephyr/kernel.h>
#include "bsp_led.h"


int main(void)
{
    while (1)
    {
        // Toggle All Leds
        bsp_led_toggle_all();

        // Suspend thread for 1000ms (Low-power sleep)
        k_msleep(1000);
    }

    return 0;
}
