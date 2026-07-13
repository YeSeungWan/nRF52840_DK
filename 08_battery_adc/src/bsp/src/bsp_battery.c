
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>


#include "bsp_battery.h"

LOG_MODULE_REGISTER(bsp_battery, LOG_LEVEL_INF);

/* Fetch the ADC channel configuration from user node in devicetree */
#define ADC_NODE DT_PATH(zephyr_user)
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET_BY_IDX(ADC_NODE, 0);

/* Internal static buffer for single-ended raw hardware conversion */
static int16_t raw_sample_buffer[1];

/**
 * @brief      Initialize the SAADC peripheral channel dedicated for battery monitoring.
 * @param[in]  None
 * @return     int 0 on success, negative error code on failure.
 */
int bsp_battery_init(void)
{
    int ret;

    if (!adc_is_ready_dt(&adc_channel))
    {
        LOG_ERR("SAADC peripheral hardware device not ready.");
        return -ENODEV;
    }

    ret = adc_channel_setup_dt(&adc_channel);
    if (ret < 0)
    {
        LOG_ERR("Failed to setup SAADC channel registers: %d", ret);
        return ret;
    }

    LOG_INF("SAADC hardware interface successfully initialized.");
    return 0;
}

/**
 * @brief       Trigger a synchronous ADC read and convert the raw count to millivolts.
 * @param[out]  out_mv Pointer to store the sampled voltage in millivolts (mV).
 * @return      int 0 on success, negative error code on failure.
 */
int bsp_battery_read_mv(uint16_t *out_mv)
{
    int ret;
    struct adc_sequence sequence = {0};

    if (out_mv == NULL)
    {
        return -EINVAL;
    }

    ret = adc_sequence_init_dt(&adc_channel, &sequence);
    if (ret < 0)
    {
        LOG_ERR("Failed to initialize ADC sequence structure: %d", ret);
        return ret;
    }

    sequence.buffer = raw_sample_buffer;
    sequence.buffer_size = sizeof(raw_sample_buffer);

    /* Trigger synchronous analog-to-digital sampling block */
    ret = adc_read(adc_channel.dev, &sequence);
    if (ret < 0)
    {
        LOG_ERR("SAADC hardware sampling read failed: %d", ret);
        return ret;
    }

    int32_t mv_val = (int32_t)raw_sample_buffer[0];

    /* Convert hardware raw steps into actual millivolts based on reference/gain */
    ret = adc_raw_to_millivolts_dt(&adc_channel, &mv_val);
    if (ret < 0)
    {
        LOG_ERR("Raw value to millivolts conversion macro failed: %d", ret);
        return ret;
    }

    /* Adjust scale factor if a voltage divider resistor network is attached */
    const uint8_t hardware_divider_ratio = 1;
    *out_mv = (uint16_t)mv_val * hardware_divider_ratio;

    return 0;
}
