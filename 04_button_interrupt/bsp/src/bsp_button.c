#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include "bsp_button.h"
#include "bsp_led.h"


// LED GPIO Configurations
static const struct gpio_dt_spec btn0 = GPIO_DT_SPEC_GET(DT_ALIAS(button0), gpios);
static const struct gpio_dt_spec btn1 = GPIO_DT_SPEC_GET(DT_ALIAS(button1), gpios);
static const struct gpio_dt_spec btn2 = GPIO_DT_SPEC_GET(DT_ALIAS(button2), gpios);
static const struct gpio_dt_spec btn3 = GPIO_DT_SPEC_GET(DT_ALIAS(button3), gpios);


// Manage Button Struct Pointer
static const struct gpio_dt_spec *buttons[] = {
    &btn0,
    &btn1,
    &btn2,
    &btn3
};


// Number of Buttons in the configuration array
static const size_t num_button = ARRAY_SIZE(buttons);


// Zephyr GPIO Callback structures for each button
static struct gpio_callback button_cb_data[4];


/**
 * @brief @brief Button Callback ISR (Executed when a button is released)
 * @todo  LED Toggle
 */
void button_released_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
    for (int i = 0; i < num_button; i++)
    {
        if (pins & BIT(buttons[i]->pin))
        {
            bsp_led_toggle(i);

			printk("Button %d pressed by user\n", i);
        }
    }
}


/**
 * @brief      Initializes the Button GPIO pins.
 * @param[in]  None
 * @retval     None
 */ 
static int bsp_button_init(void)
{
    int result;

    for (int i = 0; i < num_button; i++)
    {
		// Check if GPIO port is ready
        if (!gpio_is_ready_dt(buttons[i]))
        {
            return -ENODEV;
        }

		// Configure GPIO INput Mode
        result = gpio_pin_configure_dt(buttons[i], GPIO_INPUT);
        if (result < 0)
        {
            return result;
        }

        // Button ISR Configuration - Rising Edge (Inverted context for Active Low)
        result = gpio_pin_interrupt_configure_dt(buttons[i], GPIO_INT_EDGE_TO_INACTIVE);
        if (result < 0)
        {
            return result;
        }

		// Initialize Callback Linkage
        gpio_init_callback(&button_cb_data[i], button_released_handler, BIT(buttons[i]->pin));

		// Register Callback to the GPIO Driver
        result = gpio_add_callback(buttons[i]->port, &button_cb_data[i]);

        if (result < 0)
        {
            return result;
        }
    }

    return 0;
}


// Automatically execute bsp_button_init during system boot sequence
SYS_INIT(bsp_button_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);