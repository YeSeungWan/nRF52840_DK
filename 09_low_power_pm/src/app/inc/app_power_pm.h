#ifndef APP_POWER_PM_H_
#define APP_POWER_PM_H_

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------//
// * Extern Functions            //
// ------------------------------//
int app_power_pm_init(void);
int app_power_pm_toggle_device(const struct device *, bool);

#ifdef __cplusplus
}
#endif

#endif // APP_POWER_PM_H_
