#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_battery.h"
#include "bsp_battery.h"

LOG_MODULE_REGISTER(app_battery, LOG_LEVEL_INF);

/* Moving average filter window size for smoothing voltage dips */
#define BATT_FILTER_WINDOW_SIZE    8  

/* Nominal battery threshold definitions for CR2032 cell */
#define CR2032_MAX_MV             3000  /* 3.0V implies 100% SoC */
#define CR2032_MIN_MV             2000  /* 2.0V implies 0% SoC (Cut-off) */

/* Thread configuration for background battery polling */
#define BATTERY_THREAD_STACK_SIZE   1024
#define BATTERY_THREAD_PRIORITY     7    
#define BATTERY_CHECK_INTERVAL_MS   5000 

/* Ring buffer variables for calculating moving average metrics */
static uint16_t filter_buffer[BATT_FILTER_WINDOW_SIZE];
static uint8_t  filter_idx = 0;
static uint16_t current_filtered_mv = CR2032_MAX_MV;

/* Mutex lock to assure thread-safety across concurrent reader applications */
K_MUTEX_DEFINE(battery_data_mutex);

/* Forward declaration of the internal worker thread execution context */
static void battery_thread_entry(void *p1, void *p2, void *p3);

/* Compile-time thread registration and auto-start definition */
K_THREAD_DEFINE(battery_thread_id, BATTERY_THREAD_STACK_SIZE,
                battery_thread_entry, NULL, NULL, NULL,
                BATTERY_THREAD_PRIORITY, 0, 0);

/**
 * @brief      Initialize the application battery management buffers and underlying hardware.
 * @param[in]  None
 * @return     int 0 on success, negative error code on failure.
 */
int app_battery_init(void)
{
    /* Request low-level hardware initialization from the BSP layer */
    int ret = bsp_battery_init();
    if (ret < 0)
    {
        LOG_ERR("Dependency injection error: bsp_battery_init failed.");
        return ret;
    }

    /* Prime the filter array with nominal max cell voltage to block cold-start dips */
    for (int i = 0; i < BATT_FILTER_WINDOW_SIZE; i++)
    {
        filter_buffer[i] = CR2032_MAX_MV;
    }

    /* 
     * Safely start the background telemetry thread now that 
     * peripheral drivers and memory buffers are fully ready.
     */
    k_thread_start(battery_thread_id);

    LOG_INF("Application telemetry pipeline initialized successfully.");
    return 0;
}

/**
 * @brief      Acquire a new sample from the BSP, push to pipeline, and refresh the moving average.
 * @param[in]  None
 * @return     int 0 on success, negative error code on failure.
 */
int app_battery_process_sample(void)
{
    int ret;
    uint16_t fresh_raw_mv;

    /* Execute non-blocking capture from the isolated abstraction boundary */
    ret = bsp_battery_read_mv(&fresh_raw_mv);
    if (ret < 0)
    {
        LOG_ERR("Failed to fetch fresh hardware voltage frame.");
        return ret;
    }

    k_mutex_lock(&battery_data_mutex, K_FOREVER);

    /* Enqueue incoming metric into the sliding window architecture */
    filter_buffer[filter_idx] = fresh_raw_mv;
    filter_idx = (filter_idx + 1) % BATT_FILTER_WINDOW_SIZE;

    /* Compute the arithmetic mean across the configured window size */
    uint32_t sum = 0;
    for (int i = 0; i < BATT_FILTER_WINDOW_SIZE; i++)
    {
        sum += filter_buffer[i];
    }
    current_filtered_mv = (uint16_t)(sum / BATT_FILTER_WINDOW_SIZE);

    k_mutex_unlock(&battery_data_mutex);

    LOG_INF("Battery Telemetry - Measured: %u mV, Filtered: %u mV, SoC: %u%%", 
            fresh_raw_mv, current_filtered_mv, app_battery_get_soc(current_filtered_mv));

    return 0;
}

/**
 * @brief      Retrieve the latest smoothed voltage value cached by the moving average filter.
 * @param[in]  None
 * @return     uint16_t Smoothed battery voltage in millivolts (mV).
 */
uint16_t app_battery_get_mv(void)
{
    uint16_t localized_mv;

    k_mutex_lock(&battery_data_mutex, K_FOREVER);
    localized_mv = current_filtered_mv;
    k_mutex_unlock(&battery_data_mutex);

    return localized_mv;
}

/**
 * @brief      Map the current filtered voltage to State of Charge (SoC) capacity percentage.
 * @param[in]  voltage_mv The smoothed voltage input to convert.
 * @return     uint8_t Remaining discharge capacity percentage (0 to 100%).
 */
uint8_t app_battery_get_soc(uint16_t voltage_mv)
{
    if (voltage_mv >= CR2032_MAX_MV)
    {
        return 100;
    }

    if (voltage_mv <= CR2032_MIN_MV)
    {
        return 0;
    }

    /* Linearly map the voltage input inside the safe operational cell envelope */
    return (uint8_t)(((voltage_mv - CR2032_MIN_MV) * 100) / (CR2032_MAX_MV - CR2032_MIN_MV));
}

/**
 * @brief      Background task execution entry point looping periodically for ADC updates.
 * @param[in]  p1 Unused thread parameter.
 * @param[in]  p2 Unused thread parameter.
 * @param[in]  p3 Unused thread parameter.
 * @return     None
 */
static void battery_thread_entry(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    while (1)
    {
        /* Process and update moving average variables */
        (void)app_battery_process_sample();

        /* Sleep and yield the CPU back to the kernel scheduler */
        k_msleep(BATTERY_CHECK_INTERVAL_MS);
    }
}
