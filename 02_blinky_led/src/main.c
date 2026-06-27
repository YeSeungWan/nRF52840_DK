#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

// GPIO Port Configurations
static const struct gpio_dt_spec leds[] =
{
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};

#define NUM_LEDS ARRAY_SIZE(leds)

int main(void)
{
    // Initialize GPIO pins
    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Check if GPIO port is ready
        if (!gpio_is_ready_dt(&leds[i]))
        {
            return -1;
        }

        // Configure GPIO Output Mode
        gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_ACTIVE);
    }

    while (1)
    {
        // Toggle All Leds
        for (int i = 0; i < NUM_LEDS; i++)
        {
            gpio_pin_toggle_dt(&leds[i]);
        }
    
        // Suspend thread for 1000ms (Low-power sleep)
        k_msleep(1000);
    }

    return 0;
}