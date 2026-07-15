#ifndef APP_BATTERY_H_
#define APP_BATTERY_H_

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------//
// * Extern Functions            //
// ------------------------------//
int app_battery_init(void);
int app_battery_process_sample(void);
uint16_t app_battery_get_mv(void);
uint8_t app_battery_get_soc(uint16_t voltage_mv);

#ifdef __cplusplus
}
#endif

#endif // APP_BATTERY_H_
