#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "bsp_uart.h"
#include "app_uart.h"
#include "app_battery.h"

int main(void)
{
    LOG_INF("=== nRF52840 DK 07_uart_msgq Start ===");

    /* Low-Level Hardware Driver Layer Initialization */
    if (bsp_uart_init() != 0)
    {
        LOG_ERR("Failed to initialize bsp_uart driver!");
        return -1;
    }

    /* Telemetry Infrastructure Layer Initialization (SAADC & Filters) */
    if (app_battery_init() != 0)
    {
        LOG_ERR("Failed to initialize app_battery monitoring subsystem!");
        return -1;
    }

    /* High-Level Application Context & Thread Task Generation */
    if (app_uart_init() != 0)
    {
        LOG_ERR("Failed to initialize app_uart application context!");
        return -1;
    }


    LOG_INF("All systems initialized. Waiting for external UART packets...");
    LOG_INF("Waiting for external UART packets and monitoring battery telemetry...");

    while (1)
    {
        k_sleep(K_FOREVER);
    }

    return 0;
}
