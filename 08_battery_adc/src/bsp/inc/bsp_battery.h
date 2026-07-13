#ifndef BSP_BATTERY_H_
#define BSP_BATTERY_H_

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------//
// * Extern Functions            //
// ------------------------------//
int bsp_battery_init(void);
int bsp_battery_read_mv(uint16_t *out_mv);


#ifdef __cplusplus
}
#endif


#endif /* BSP_BATTERY_H_ */