#ifndef APP_WATCHDOG_H_
#define APP_WATCHDOG_H_

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------//
// * Extern Functions            //
// ------------------------------//
int app_watchdog_init(void);
void app_watchdog_feed(void);

#ifdef __cplusplus
}
#endif

#endif // APP_WATCHDOG_H_
