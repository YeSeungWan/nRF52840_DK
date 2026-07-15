#ifndef BSP_UART_H_
#define BSP_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------//
// * Extern Variables            //
// ------------------------------//
extern struct k_msgq uart_msgq;

// ------------------------------//
// * Extern Functions            //
// ------------------------------//
int bsp_uart_init(void);

int UART_Read(uint8_t *, size_t, k_timeout_t);
int UART_Write(const uint8_t *, size_t, k_timeout_t);
const struct device *bsp_uart_get_dev(void);


#ifdef __cplusplus
}
#endif

#endif // BSP_UART_H_
