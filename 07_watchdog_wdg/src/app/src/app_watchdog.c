#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_wdg, LOG_LEVEL_INF);

#include "app_watchdog.h"

/* Resolve watchdog node from devicetree alias */
#define WDT_NODE DT_ALIAS(watchdog0)

#if !DT_NODE_HAS_STATUS_OKAY(WDT_NODE)
#define WDT_NODE DT_NODELABEL(wdt)
#endif

static const struct device *const wdt_dev = DEVICE_DT_GET(WDT_NODE);
static int wdt_channel_id;

int app_watchdog_init(void)
{
    struct wdt_timeout_cfg wdt_config = {0};
    int ret;

    /* Verify if the hardware device instance is ready */
    if (!device_is_ready(wdt_dev))
    {
        LOG_ERR("Watchdog peripheral device not ready");
        return -ENODEV;
    }

    /* Configure watchdog timeout parameters (2000ms window) */
    wdt_config.flags = WDT_FLAG_RESET_SOC;             /* Trigger full SoC hardware reset on timeout */
    wdt_config.window.min = 0U;                        /* Non-windowed mode: allow refresh at any time */
    wdt_config.window.max = 2000U;                     /* 2000ms expiration threshold */
    wdt_config.callback = NULL;                        /* nRF52 resets immediately; callback is not recommended */

    /* Install the timeout configuration channel */
    wdt_channel_id = wdt_install_timeout(wdt_dev, &wdt_config);
    if (wdt_channel_id < 0)
    {
        LOG_ERR("Failed to install watchdog timeout: %d", wdt_channel_id);
        return wdt_channel_id;
    }

    /* Setup and start the watchdog timer */
    /* WDT_OPT_PAUSE_IN_SLEEP ensures WDT pauses when SoC enters low-power modes (Step 09) */
    ret = wdt_setup(wdt_dev, WDT_OPT_PAUSE_IN_SLEEP);
    if (ret < 0)
    {
        LOG_ERR("Watchdog setup failed: %d", ret);
        return ret;
    }

    LOG_INF("Hardware Watchdog active (Timeout: %d ms, Ch: %d)", 2000, wdt_channel_id);
    return 0;
}

void app_watchdog_feed(void)
{
    /* Clear the counter of the designated watchdog channel */
    wdt_feed(wdt_dev, wdt_channel_id);
}