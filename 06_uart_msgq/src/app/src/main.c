#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#include "bsp_uart.h"
#include "app_uart.h"

int main(void)
{
    LOG_INF("=== nRF52840 DK 06_uart_msgq Start ===");

    if (bsp_uart_init() != 0)
    {
        LOG_ERR("Failed to initialize bsp_uart driver!");
        return -1;
    }

    
    if (app_uart_init() != 0)
    {
        LOG_ERR("Failed to initialize app_uart application context!");
        return -1;
    }

    LOG_INF("All systems initialized. Waiting for external UART packets...");

    while (1)
    {
        k_sleep(K_FOREVER);
    }

    return 0;
}
