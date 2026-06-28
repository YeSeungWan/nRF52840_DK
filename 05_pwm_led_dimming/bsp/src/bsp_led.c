#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/init.h>
#include "bsp_led.h"


/* LED PWM Configurations */
static const struct pwm_dt_spec pwm_leds[] = {
    PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0)),
    PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1)),
    PWM_DT_SPEC_GET(DT_ALIAS(pwm_led2)),
    PWM_DT_SPEC_GET(DT_ALIAS(pwm_led3)),
};

/* Number of LEDs in the configuration array */
#define PWM_LED_NUM_MAX         ARRAY_SIZE(pwm_leds)

/* PWM brightness increment step (10%) */
#define PWM_INCREASE_VALUE      10

/* Maximum PWM brightness limit (100%) */
#define PWM_MAX_VALUE           100

/* Array to maintain the current brightness state (0-100%) for each LED */
static uint8_t led_brightness_states[PWM_LED_NUM_MAX] = {0, };



/**
 * @brief      Set the brightness of a specific LED using PWM duty cycle.
 * @param[in]  target   The index of the target LED.
 * @param[in]  value    Brightness level from 0 to 100 percent.
 * @retval     0        On success
 * @retval     Negative error code on failure
 */
int bsp_pwm_led_set(uint8_t target, uint8_t value)
{
    /* 1 kHz period in nanoseconds (1,000,000 ns) */
    uint32_t period = PWM_MSEC(1); 

    /* Calculate the high-pulse width based on the target brightness percentage */
    /* Formula: Pulse Width = Period * (Brightness / 100) */
    uint32_t pulse = (period * value) / 100;

    /* nRF52840 DK LEDs are Active-Low (Low = ON, High = OFF). */
    /* To dim the LED properly, we need to invert the pulse width for the hardware. */
    uint32_t inverted_pulse = period - pulse;

    /* Apply the calculated period and pulse width to the hardware registers */
    return pwm_set_dt(&pwm_leds[target], period, inverted_pulse);
}


/**
 * @brief      Increase the brightness of a specific LED by 10%. Loops back to 0% after 100%.
 * @param[in]  target   The index of the target LED.
 * @retval     None
 */
void bsp_pwm_led0_increase()
{
    uint8_t result;

    /* Increase the brightness value state */
    led_brightness_states[0] += PWM_INCREASE_VALUE;

    // Rollback 0%
    if (led_brightness_states[0] > PWM_MAX_VALUE)
    {
        led_brightness_states[0] = 0;
    }

    /* Apply the updated state to the PWM hardware */
    result = bsp_pwm_led_set(0, led_brightness_states[0]);
    
    /* Print debug log to serial console */
    if (result < 0)
    {
        printk("[LED] Error: Failed to set LED %d PWM (err: %d)\n", 0, result);
    }
    else
    {
        printk("[LED] LED %d brightness increased to %d%%\n", 0, led_brightness_states[0]);
    }
}


/**
 * @brief      Increase the brightness of a specific LED by 10%. Loops back to 0% after 100%.
 * @param[in]  target   The index of the target LED.
 * @retval     None
 */
void bsp_pwm_led_increase(uint8_t target)
{
    uint8_t result;

    /* Target index out of bounds check */
    if (target >= PWM_LED_NUM_MAX)
    {
        return;
    }

    /* Increase the brightness value state */
    led_brightness_states[target] += PWM_INCREASE_VALUE;

    // Rollback 0%
    if (led_brightness_states[target] > PWM_MAX_VALUE)
    {
        led_brightness_states[target] = 0;
    }

    /* Apply the updated state to the PWM hardware */
    result = bsp_pwm_led_set(target, led_brightness_states[target]);
    
    /* Print debug log to serial console */
    if (result < 0)
    {
        printk("[LED] Error: Failed to set LED %d PWM (err: %d)\n", target, result);
    }
    else
    {
        printk("[LED] LED %d brightness increased to %d%%\n", target, led_brightness_states[target]);
    }
}


/**
 * @brief      Initializes the PWM LED hardware instances
 * @param[in]  None
 * @retval     0         On success
 * @retval     -ENODEV   If the PWM device driver is not ready
 */ 
static int bsp_pwm_led_init(void)
{
    for (int i = 0; i < PWM_LED_NUM_MAX; i++)
    {
        if (!pwm_is_ready_dt(&pwm_leds[i]))
        {
            return -ENODEV;
        }
    }
    return 0;
}


/* Automatically execute bsp_pwm_led_init during system boot sequence */
SYS_INIT(bsp_pwm_led_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
