#ifndef BSP_LED_H_
#define BSP_LED_H_

#ifdef __cplusplus
extern "C" {
#endif


// ------------------------------//
// * Extern Function             //
// ------------------------------//
void bsp_pwm_led_increase(uint8_t);
void bsp_pwm_led0_increase(void);

#ifdef __cplusplus
}
#endif

#endif // BSP_LED_H_
