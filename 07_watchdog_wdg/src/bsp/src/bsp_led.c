#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/init.h>
#include "bsp_led.h"

LOG_MODULE_REGISTER(bsp_led, CONFIG_LOG_DEFAULT_LEVEL);

/* LED PWM Configurations */
static const struct pwm_dt_spec pwm_leds[] =
{
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
    /* Target index out of bounds check */
    if (target >= PWM_LED_NUM_MAX)
    {
        return -EINVAL;
    }

    /* Input scaling protection */
    if (value > PWM_MAX_VALUE)
    {
        value = PWM_MAX_VALUE;
    }
    
    value = 100 - value;

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
 * @param[in]  target The index of the target LED.
 * @retval     None
 */
void bsp_pwm_led_increase(uint8_t target)
{
    int result;

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
 * @brief      Retrieves the maximum number of configurable physical PWM LED instances.
 * @param[in]  None
 * @retval     uint8_t The total capacity count defined by PWM_LED_NUM_MAX.
 */
uint8_t bsp_get_led_num(void)
{
    return PWM_LED_NUM_MAX;
}


/**
 * @brief      Obtains the base memory address of the global PWM LED array.
 * @param[in]  None
 * @retval     uint8_t* Pointer to the mutable LED brightness array context.
 */
uint8_t *bsp_get_all_pwm_led(void)
{
    return led_brightness_states;
}


/**
 * @brief      Retrieves the current PWM level of a specific targeted LED instance.
 * @param[in]  target Target index of the physical LED channel to query.
 * @retval     uint8_t The extracted brightness value (0-100%).
 */
uint8_t bsp_get_individual_pwm_led(uint8_t target)
{
    if (target >= PWM_LED_NUM_MAX)
    {
        return 0;
    }

    return led_brightness_states[target];
}


/**
 * @brief      Updates the dimming levels for all physical PWM LED instances at once.
 * @note       Accepts dimming scales in percentages (0-100%). Clamps raw inputs.
 * @param[in]  pwm_arr Pointer to the source array containing percentage values (0-100%).
 * @param[in]  arr_num The total configuration count of elements inside the source array.
 * @retval     None
 */
void bsp_set_all_pwm_led(uint8_t *pwm_arr, uint8_t arr_num)
{
    int result;

    /* Null pointer guard to prevent system panic */
    if (pwm_arr == NULL || arr_num == 0)
    {
        return;
    }

    /* Prevent deep memory corruption by clamping the maximum loop range */
    if (arr_num > PWM_LED_NUM_MAX)
    {
        arr_num = PWM_LED_NUM_MAX;
    }

    for (uint8_t i = 0; i < arr_num; i++)
    {
        if (pwm_arr[i] > PWM_MAX_VALUE)
        {
            led_brightness_states[i] = PWM_MAX_VALUE;
        }
        else
        {
            led_brightness_states[i] = pwm_arr[i];
        }

        /* Apply the updated state to the PWM hardware */
        result = bsp_pwm_led_set(i, led_brightness_states[i]);

        if (result < 0)
        {
            LOG_ERR("[LED] Error: Failed to set LED %d PWM (err: %d)", i, result);
        }
    }
}


/**
 * @brief      Sets the specific target PWM LED channel to a designated dimming value.
 * @note       Converts 0-100% input scale directly into hardware duty cycle ranges.
 * @param[in]  target Target index of the physical LED channel to update.
 * @param[in]  pwm_value Target percentage brightness level value (0-100%).
 * @retval     None
 */
void bsp_set_individual_pwm_led(uint8_t target, uint8_t pwm_value)
{
    int result;

    if (target >= PWM_LED_NUM_MAX)
    {
        return;
    }

    if (pwm_value > PWM_MAX_VALUE)
    {
        pwm_value = PWM_MAX_VALUE;
    }

    led_brightness_states[target] = pwm_value;

    result = bsp_pwm_led_set(target, pwm_value);

    if (result < 0)
    {
        LOG_ERR("[LED] Error: Failed to set LED %d PWM (err: %d)", target, result);
    }
}


/**
 * @brief      Initializes the PWM LED hardware instances.
 * @param      None
 * @retval     0 On success
 * @retval     -ENODEV If the PWM device driver is not ready
 */ 
static int bsp_pwm_led_init(void)
{
    for (uint8_t i = 0; i < PWM_LED_NUM_MAX; i++)
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