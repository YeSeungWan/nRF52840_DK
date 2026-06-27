#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include "bsp_led.h"


// LED GPIO Configurations
static const struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios),
};

// Number of LEDs in the configuration array
static const size_t num_leds = ARRAY_SIZE(leds);


/**
 * @brief Toggles all configured LEDs.
 * @param[in]  None
 * @retval     None
 */
void bsp_led_toggle_all(void)
{
    for (int i = 0; i < num_leds; i++)
    {
        gpio_pin_toggle_dt(&leds[i]);
    }
}


/**
 * @brief      Toggle a specific LED by its index.
 * @param[in]  target  The index of the target LED to toggle.
 * @retval     None
 */
void bsp_led_toggle(uint8_t target)
{
    gpio_pin_toggle_dt(&leds[target]);
}


/**
 * @brief Initializes the LED GPIO pins.
 * @param[in]  None
 * @retval     None
 */ 
static int bsp_led_init(void)
{
    for (int i = 0; i < num_leds; i++) {
        if (!gpio_is_ready_dt(&leds[i])) { return -ENODEV; }
        gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_ACTIVE);
    }
    return 0;
}


// Automatically execute bsp_led_init during system boot sequence
SYS_INIT(bsp_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
