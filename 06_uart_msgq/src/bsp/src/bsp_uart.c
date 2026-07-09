#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include "bsp_uart.h"

LOG_MODULE_REGISTER(bsp_uart, LOG_LEVEL_INF);

/* Fetch the UART0 device instance from the devicetree */
static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

/* Define a message queue for 1-byte data with a depth of 128 elements */
K_MSGQ_DEFINE(uart_msgq, sizeof(uint8_t), 128, 4);


/**
 * @brief UART hardware RX interrupt callback routine (ISR)
 */
static void uart_cb(const struct device *dev, void *user_data)
{
    uint8_t recv_byte;

    /* Verify and update the interrupt status registers */
    uart_irq_update(dev);

    /* Check if RX data is ready in the FIFO */
    if (uart_irq_rx_ready(dev))
    {
        while (uart_fifo_read(dev, &recv_byte, 1) > 0)
        {
            /* Push data into the queue. Must use K_NO_WAIT inside ISR context. */
            if (k_msgq_put(&uart_msgq, &recv_byte, K_NO_WAIT) != 0)
            {
                LOG_WRN("UART RX Queue full! Data dropped.");
            }
        }
    }
}


/**
 * @brief      Initialize BSP UART peripheral and enable interrupts
 * @param[in]  None
 * @return     0 on success, negative error code on failure
 */
int bsp_uart_init(void)
{
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready!");
        return -ENODEV;
    }

    /* Register the user callback function for UART interrupts */
    uart_irq_callback_user_data_set(uart_dev, uart_cb, NULL);
    
    /* Enable RX interrupts at the hardware level */
    uart_irq_rx_enable(uart_dev);

    LOG_INF("UART Driver Initialized successfully.");

    return 0;
}


/**
 * @brief      Production API to read data from the UART RX queue
 * @param[in]  buf Pointer to the destination buffer in application layer
 * @param[in]  len Number of bytes to read
 * @param[in]  timeout K_FOREVER, K_NO_WAIT, or specific millisecond timeout
 * @return     Number of bytes read on success, negative error code on failure
 */
int UART_Read(uint8_t *buf, size_t len, k_timeout_t timeout)
{
    if (buf == NULL || len == 0)
    {
        return -EINVAL;
    }

    size_t read_bytes = 0;

    for (size_t i = 0; i < len; i++) {
        /* Retrieve 1 byte from the queue. Thread sleeps automatically if queue is empty. */
        if (k_msgq_get(&uart_msgq, &buf[i], timeout) != 0) {
            /* If timeout occurs mid-stream, return the number of bytes processed so far */
            return (read_bytes == 0) ? -ETIMEDOUT : (int)read_bytes;
        }
        read_bytes++;
    }

    return (int)read_bytes;
}

/**
 * @brief      Production API to transmit data over UART
 * @param[in]  buf Target to send Buffer
 * @param[in]  len Buffer Size
 * @param[in]  timeout K_FOREVER, K_NO_WAIT, or specific millisecond timeout
 * @return     Number of bytes transmitted on success, negative error code on failur
 */
int UART_Write(const uint8_t *buf, size_t len, k_timeout_t timeout)
{
    (void)timeout;

    if (buf == NULL || len == 0)
    {
        return -EINVAL;
    }

    int log = uart_fifo_fill(uart_dev, buf, len);
        
    return log;
}
