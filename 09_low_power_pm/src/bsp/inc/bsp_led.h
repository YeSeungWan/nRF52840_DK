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

const struct device *bsp_pwm_get_dev(void);
uint8_t bsp_get_led_num(void);
uint8_t *bsp_get_all_pwm_led(void);
uint8_t bsp_get_individual_pwm_led(uint8_t);

void bsp_set_all_pwm_led(uint8_t *, uint8_t);
void bsp_set_individual_pwm_led(uint8_t, uint8_t);


#ifdef __cplusplus
}
#endif

#endif // BSP_LED_H_
