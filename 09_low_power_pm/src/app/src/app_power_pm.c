
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>

#include "app_power_pm.h"
#include "bsp_led.h"
#include "bsp_battery.h"
#include "bsp_uart.h"


LOG_MODULE_REGISTER(app_power_pm, LOG_LEVEL_INF);


/**
 * @brief       Callback invoked when the system is about to ENTER a low power state.
 * @param[in]   state The low power state the system is transitioning into.
 * @retval      None
 */
static void app_pm_state_entry_cb(enum pm_state state)
{
    switch (state)
    {
        case PM_STATE_RUNTIME_IDLE:
            LOG_INF("System PM: Entering RUNTIME IDLE (Light Sleep)");
            break;
        case PM_STATE_SUSPEND_TO_RAM:
            LOG_INF("System PM: Entering SUSPEND TO RAM (Deep Sleep)");
            break;
        default:
            LOG_INF("System PM: Entering State (%d)", state);
            break;
    }
}


/**
 * @brief       Callback invoked when the system has just EXITED a low power state.
 * @param[in]   state        The specific low power state the system has just departed from.
 * @retval      None
 */
static void app_pm_state_exit_cb(enum pm_state state)
{
    switch (state)
    {
        case PM_STATE_RUNTIME_IDLE:
            LOG_INF("System PM: EXIT RUNTIME IDLE -> Resuming ACTIVE State");
            break;
        case PM_STATE_SUSPEND_TO_RAM:
            LOG_INF("System PM: EXIT SUSPEND TO RAM -> Resuming ACTIVE State");
            break;
        default:
            /* Defensive handling for undocumented or platform-specific state escapes */
            LOG_INF("System PM: EXIT State (%d) -> Resuming ACTIVE State", state);
            break;
    }
}


/* Static instantiation compatible with Zephyr 3.7+ / NCS v3.3.0 */
static struct pm_notifier pm_notifier_cb =
{
    .state_entry = app_pm_state_entry_cb,
    .state_exit = app_pm_state_exit_cb,
};


/**
 * @brief       Dynamically toggles the power state of a specific peripheral device.
 * @param[in]   dev     Pointer to the device structure to control.
 * @param[in]   suspend true to suspend (power off), false to resume (power on).
 * @return      int 0 on success, negative errno on failure.
 */
int app_power_pm_toggle_device(const struct device *dev, bool suspend)
{
    int ret;
    
    if (!device_is_ready(dev))
    {
        LOG_ERR("Device %s is not ready for PM transition", dev->name);
        return -ENODEV;
    }

    if (suspend)
    {
        ret = pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
        if (ret == 0)
        {
            LOG_INF("Device PM: %s successfully SUSPENDED", dev->name);
        }
        else
        {
            LOG_ERR("Failed to suspend device %s (err: %d)", dev->name, ret);
        }
    }
    else
    {
        ret = pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
    
        if (ret == 0)
        {
            LOG_INF("Device PM: %s successfully RESUMED", dev->name);
        }
        else
        {
            LOG_ERR("Failed to resume device %s (err: %d)", dev->name, ret);
        }
    }

    return ret;
}


/**
 * @brief      Initialize the System Power Management components and register PM notifiers.
 * @param[in]  None
 * @return     int 0 on success, negative error code on failure.
 */
int app_power_pm_init(void)
{
    int ret = 0;

    /* Register system power management transition notifier */
    pm_notifier_register(&pm_notifier_cb);
    LOG_INF("System PM Notifier Registered Successfully.");

    const struct device *pwm_dev = bsp_pwm_get_dev();

    if (pwm_dev != NULL)
    {
        app_power_pm_toggle_device(pwm_dev, true);
    }

    const struct device *adc_dev = bsp_battery_get_dev();
    if (adc_dev != NULL)
    {
        app_power_pm_toggle_device(adc_dev, true);
    }

    const struct device *uart_dev = bsp_uart_get_dev();

    if(uart_dev != NULL)
    {
        app_power_pm_toggle_device(uart_dev, true);
    }

    return ret;
}
