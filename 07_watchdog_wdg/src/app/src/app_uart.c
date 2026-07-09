#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_uart, LOG_LEVEL_INF);

#include "app_uart.h"
#include "bsp_uart.h"
#include "bsp_led.h"
#include "app_watchdog.h"

#define APP_UART_STACK_SIZE             2560
#define APP_UART_PRIORITY               7

#define PACKET_SOF_BYTE                 0xAA
#define MAX_PAYLOAD_SIZE                2560

/* Constants derived directly from the structure layout */
#define PACKET_HEADER_SIZE              (offsetof(struct app_uart_packet, cmd))
#define PACKET_MIN_SIZE                 (PACKET_HEADER_SIZE + 2)
#define PACKET_MAX_SIZE                 (sizeof(struct app_uart_packet) + 1)

/* Response Formatting Bitmasks */
#define PROTOCOL_RSP_BITMASK            0x80
#define PROTOCOL_SUB_ID_MASK            0x0F

/* Payload Layout Explicit Offsets */
#define PL_IDX_LED_COUNT                0  /* Payload index representing number of LEDs */
#define PL_IDX_LED_DATA_START           1  /* Target array block stream start offset */
#define PL_IDX_INDIVIDUAL_VAL           0  /* Single LED target value offset */

/* System Status & Control Commands (0x01 ~ 0x0F) */
#define CMD_SYS_REQ_STATE               0x01
#define CMD_SYS_CTRL_INDIVIDUAL         0x02
#define CMD_SYS_CTRL_ALL                0x03

/* Firmware OTA Sequences (0x10 ~ 0x1F) */
#define CMD_OTA_START                   0x10
#define CMD_OTA_DATA                    0x11
#define CMD_OTA_END                     0x12

#define UART_TX_TIMEOUT_TICKS           10000


/* UART FSM Step */
typedef enum
{
    APP_STATE_IDLE = 0,
    APP_STATE_PARSE,                    /* Frame boundary slicing & accumulation */
    APP_STATE_VALIDATE,                 /* Integrity, Checksum, or KISA Crypto check */
    APP_STATE_PROCESSING,               /* Command execution context */
    APP_STATE_MAX
} app_state_t;


/**
 * @brief Protocol Packet Structure
 */
struct app_uart_packet
{
    uint8_t sof;                        /* Start of Frame (Offset 0) */
    uint8_t device_id;                  /* Device Identification (Offset 1) */
    uint8_t sub_id;                     /* Sub-system Identification (Offset 2) */
    uint16_t length;                    /* Pure payload length or tracking index (Offset 3) */
    uint8_t cmd;                        /* Command ID (Offset 5) */
    uint8_t payload[MAX_PAYLOAD_SIZE];  /* Variable data block */
} __packed;

/**
 * @brief FSM Instance Context Structure for Encapsulation
 * This isolates state variables to enable multi-port reentrancy.
 */
struct app_uart_context
{
    app_state_t current_state;          /* Current tracking state of the specific channel */
    uint8_t packet_buf[PACKET_MAX_SIZE];/* Dedicated receive buffer allocation */
    size_t packet_idx;                  /* Stream byte accumulation tracker */
    uint16_t expected_len;              /* Extracted dynamic frame size boundary */
    struct k_timer rx_timeout_timer;
};


/* Function pointer type passing the context instance instance */
typedef void (*fsm_handler_t)(struct app_uart_context *ctx, uint8_t rx_byte);

/* Static instantiation of the primary SoC UART channel context */
static struct app_uart_context g_soc_uart_ctx;

/* Forward declarations matching the newly encapsulated context signatures */
static void handle_state_idle(struct app_uart_context *ctx, uint8_t rx_byte);
static void handle_state_parse(struct app_uart_context *ctx, uint8_t rx_byte);
static void handle_state_validate(struct app_uart_context *ctx, uint8_t rx_byte);
static void handle_state_processing(struct app_uart_context *ctx, uint8_t rx_byte);

/* Dynamic O(1) Function Pointer Table mapped with encapsulated footprints */
static const fsm_handler_t fsm_table[APP_STATE_MAX] =
{
    [APP_STATE_IDLE]       = handle_state_idle,
    [APP_STATE_PARSE]      = handle_state_parse,
    [APP_STATE_VALIDATE]   = handle_state_validate,
    [APP_STATE_PROCESSING] = handle_state_processing
};


/**
 * @brief          Builds and transmits a response frame over the physical UART bus.
 * @param[in,out]  ctx     Pointer to the isolated UART channel context instance.
 * @param[in]      cmd     Target Response Command ID to transmit.
 * @retval         None
 */
static void app_uart_state_tx_res(struct app_uart_context *ctx, uint8_t cmd)
{
    struct app_uart_packet *rx_pkt = (struct app_uart_packet *)ctx->packet_buf;
    struct app_uart_packet tx_pkt;

	uint8_t led_num = bsp_get_led_num();
    const uint8_t *all_led_states = bsp_get_all_pwm_led();

	/* Fill Frame */
    tx_pkt.sof        = PACKET_SOF_BYTE;
    tx_pkt.device_id  = rx_pkt->device_id;
    tx_pkt.sub_id     = rx_pkt->sub_id;
    tx_pkt.length     = (uint16_t)(led_num + 1);
    tx_pkt.cmd        = (cmd | PROTOCOL_RSP_BITMASK);
	
	/* Fill Payload */
	tx_pkt.payload[PL_IDX_LED_COUNT] = led_num;

	for (uint8_t i = 0; i < led_num; i++)
    {
        tx_pkt.payload[PL_IDX_LED_DATA_START + i] = all_led_states[i];
    }

    /* Calculate Checksum */
    uint8_t checksum = 0;
    uint8_t *tx_raw = (uint8_t *)&tx_pkt;
    size_t total_len = PACKET_MIN_SIZE + tx_pkt.length;

    for (size_t i = 0; i < (total_len - 1); i++)
    {
        checksum ^= tx_raw[i];
    }

    tx_raw[total_len - 1] = checksum;

    /* Send message to UART */
    uint8_t log = UART_Write(tx_raw, total_len, K_TICKS(UART_TX_TIMEOUT_TICKS));

#ifndef RELEASE
	LOG_INF("UART TX SEND LOG: %d", log);
#endif
}


/* ========================================================================== */
/* 1. System Status & Control Command Handlers (0x01 ~ 0x03)                */
/* ========================================================================== */

/**
 * @brief          Processes the request for the current system state.
 * @param[in,out]  ctx      Pointer to the isolated UART channel context instance.
 * @retval         None
 */
static void handle_cmd_req_state(struct app_uart_context *ctx)
{
	app_uart_state_tx_res(ctx, CMD_SYS_REQ_STATE);
}


/**
 * @brief          Executes individual actuator/module action requests.
 * @param[in,out]  ctx      Pointer to the isolated UART channel context instance.
 * @retval         None
 */
static void handle_cmd_sys_ctrl_individual(struct app_uart_context *ctx)
{
    struct app_uart_packet *rx_pkt = (struct app_uart_packet *)ctx->packet_buf;    
    uint8_t led_target = (rx_pkt->sub_id & PROTOCOL_SUB_ID_MASK);

    /* bsp internal safeguards will clamp values or bounds inside */
    bsp_set_individual_pwm_led(led_target, rx_pkt->payload[0]);
    
	app_uart_state_tx_res(ctx, CMD_SYS_REQ_STATE);
}


/**
 * @brief          Executes global/all actuator action requests simultaneously.
 * @param[in,out]  ctx      Pointer to the isolated UART channel context instance.
 * @retval         None
 */
static void handle_cmd_sys_ctrl_all(struct app_uart_context *ctx)
{
    struct app_uart_packet *rx_pkt = (struct app_uart_packet *)ctx->packet_buf;
    uint8_t max_leds = bsp_get_led_num();
    uint16_t led_num = rx_pkt->payload[PL_IDX_LED_COUNT];
  
    uint8_t buf[4];

    if (led_num > max_leds)
    {
        led_num = max_leds;
    }

    for (uint8_t i = 0; i < led_num; i++)
    {
        buf[i] = rx_pkt->payload[PL_IDX_LED_DATA_START + i];
    }

    bsp_set_all_pwm_led(buf, led_num);
    

	app_uart_state_tx_res(ctx, CMD_SYS_REQ_STATE);
}


/* ========================================================================== */
/* 2. Firmware OTA Sequence Handlers (0x10 ~ 0x12)                          */
/* ========================================================================== */

/**
 * @brief          Initializes the OTA session and performs pre-validation handshaking.
 * @param[in,out]  ctx      Pointer to the isolated UART channel context instance.
 * @retval         None
 */
static void handle_cmd_ota_start(struct app_uart_context *ctx)
{
    (void)ctx;
    /* TODO: Flash memory sector unlock sequences & signature handshakes */
}


/**
 * @brief          Streams incoming raw binary image blocks directly into flash memory buffers.
 * @param[in,out]  ctx      Pointer to the isolated UART channel context instance.
 * @retval         None
 */
static void handle_cmd_ota_data(struct app_uart_context *ctx)
{
    (void)ctx;
    /* TODO: Continuous peripheral binary storage stream pipeline */
}


/**
 * @brief          Finalizes the OTA session, triggering image checksum verification and reboot.
 * @param[in,out]  ctx      Pointer to the isolated UART channel context instance.
 * @retval         None
 */
static void handle_cmd_ota_end(struct app_uart_context *ctx)
{
    (void)ctx;
    /* TODO: Structural integrity check & Zephyr dual-bank boot swap triggers */
}


/**
 * @brief          Handles the IDLE state logic using localized object instance tracking.
 * @param[in,out]  ctx      Pointer to the isolated execution channel context.
 * @param[in]      rx_byte  Received byte from the physical UART bus.
 * @retval         None
 */
static void handle_state_idle(struct app_uart_context *ctx, uint8_t rx_byte)
{
    if (rx_byte == PACKET_SOF_BYTE)
    {
#ifndef RELEASE
        LOG_INF("UART FSM STATE: Change(IDLE -> PARSE)");
#endif
        ctx->packet_idx = 0;
        ctx->expected_len = 0;
        ctx->packet_buf[ctx->packet_idx++] = rx_byte;

        /* Activate dynamic 10ms boundary check watchdog timer */
        k_timer_start(&ctx->rx_timeout_timer, K_MSEC(10), K_NO_WAIT);
        ctx->current_state = APP_STATE_PARSE;
    }
}


/**
 * @brief          Handles the PARSE state logic using localized object instance tracking.
 * @param[in,out]  ctx      Pointer to the isolated execution channel context.
 * @param[in]      rx_byte  Received byte from the physical UART bus.
 * @retval         None
 */
static void handle_state_parse(struct app_uart_context *ctx, uint8_t rx_byte)
{
    struct app_uart_packet *pkt = (struct app_uart_packet *)ctx->packet_buf;

    if (ctx->packet_idx >= PACKET_MAX_SIZE)
    {
#ifndef RELEASE
        LOG_ERR("Buffer Overflow!");
        LOG_INF("UART FSM STATE: Change(PARSE -> IDLE)");
#endif
        ctx->current_state = APP_STATE_IDLE;
        return;
    }

    ctx->packet_buf[ctx->packet_idx++] = rx_byte;

#ifndef RELEASE
    LOG_INF("UART FSM PARSE: 0x%02X Receive", rx_byte);
#endif

    if (ctx->packet_idx == PACKET_HEADER_SIZE)
    {
        ctx->expected_len = pkt->length + PACKET_MIN_SIZE;
        
        if (ctx->expected_len > PACKET_MAX_SIZE)
        {
            LOG_WRN("Malformed packet length: %d. Purging.", ctx->expected_len);
            LOG_INF("UART FSM STATE: Change(PARSE -> IDLE)");
            ctx->current_state = APP_STATE_IDLE;
            return;
        }
    }

    if (ctx->expected_len > 0 && ctx->packet_idx == ctx->expected_len)
    {
        k_timer_stop(&ctx->rx_timeout_timer);
        ctx->current_state = APP_STATE_VALIDATE;
#ifndef RELEASE
        LOG_INF("UART FSM STATE: Change(PARSE -> VALIDATE)");
#endif
        fsm_table[ctx->current_state](ctx, 0);
    }
}


/**
 * @brief          Handles the VALIDATE state logic using localized object instance tracking.
 * @param[in,out]  ctx      Pointer to the isolated execution channel context.
 * @param[in]      rx_byte  Unused parameter in this state scope.
 * @retval         None
 */
static void handle_state_validate(struct app_uart_context *ctx, uint8_t rx_byte)
{
    (void)rx_byte;
    uint8_t calc_checksum = 0;

    for (size_t i = 0; i < (ctx->expected_len - 1); i++)
    {
        calc_checksum ^= ctx->packet_buf[i];
    }

    if (calc_checksum == ctx->packet_buf[ctx->expected_len - 1])
    {
#ifndef RELEASE
        LOG_INF("UART FSM VALIDATE: Validate Success.");
        LOG_INF("UART FSM STATE: Change(VALIDATE -> PROCESSING)");
#endif
        ctx->current_state = APP_STATE_PROCESSING;
        fsm_table[ctx->current_state](ctx, 0);
    }
    else
    {
#ifndef RELEASE
        LOG_ERR("UART FSM VALIDATE: Checksum mismatch. (0x%02X)", calc_checksum);
        LOG_INF("UART FSM STATE: Change(VALIDATE -> IDLE)");
#endif
        ctx->current_state = APP_STATE_IDLE;
    }
}


/**
 * @brief          Handles the PROCESSING state logic using localized object instance tracking.
 * @param[in,out]  ctx      Pointer to the isolated execution channel context.
 * @param[in]      rx_byte  Unused parameter in this state scope.
 * @retval         None
 */
static void handle_state_processing(struct app_uart_context *ctx, uint8_t rx_byte)
{
    (void)rx_byte;
    struct app_uart_packet *pkt = (struct app_uart_packet *)ctx->packet_buf;

#ifndef RELEASE
    LOG_INF("UART FSM PROCESSING: CMD 0x%02X Execute", pkt->cmd);
#endif

    switch (pkt->cmd)
    {
        case CMD_SYS_REQ_STATE:
            handle_cmd_req_state(ctx);
            break;
        case CMD_SYS_CTRL_INDIVIDUAL:
            handle_cmd_sys_ctrl_individual(ctx);
            break;
        case CMD_SYS_CTRL_ALL:
            handle_cmd_sys_ctrl_all(ctx);
            break;
        case CMD_OTA_START:
            handle_cmd_ota_start(ctx);
            break;
        case CMD_OTA_DATA:
            handle_cmd_ota_data(ctx);
            break;
        case CMD_OTA_END:
            handle_cmd_ota_end(ctx);
            break;
        default:
            LOG_WRN("Unsupported CMD ID: 0x%02X", pkt->cmd);
            break;
    }

    ctx->current_state = APP_STATE_IDLE;

#ifndef RELEASE
    LOG_INF("UART FSM STATE: Change(PROCESSING -> IDLE)");
#endif
}


/**
 * @brief          Top-level Central Router for the UART Finite State Machine (FSM).
 * @param[in,out]  ctx      Pointer to the isolated execution channel context.
 * @param[in]      rx_byte  Incoming raw byte streamed directly from the receiver stream.
 * @retval         None
 */
static void app_uart_soc_fsm(struct app_uart_context *ctx, uint8_t rx_byte)
{
    if (ctx == NULL || ctx->current_state >= APP_STATE_MAX)
    {
        if (ctx != NULL)
        {
            ctx->current_state = APP_STATE_IDLE;
        }
        return;
    }

    /* Pass the targeted instanced context dynamic reference block */
    fsm_table[ctx->current_state](ctx, rx_byte);
}


static void uart_rx_timeout_handler(struct k_timer *timer)
{

    struct app_uart_context *ctx = CONTAINER_OF(timer, struct app_uart_context, rx_timeout_timer);
    
    ctx->current_state = APP_STATE_IDLE;
    ctx->packet_idx = 0;
    ctx->expected_len = 0;
    
    LOG_WRN("UART FSM: RX Packet Timeout Triggered. Resetting to IDLE.");
}


void app_uart_fsm_init(struct app_uart_context *ctx)
{
    k_timer_init(&ctx->rx_timeout_timer, uart_rx_timeout_handler, NULL);
}


/**
 * @brief          Application UART Task Loop.
 * @param[in]      p1       User thread parameter 1 (Unused).
 * @param[in]      p2       User thread parameter 2 (Unused).
 * @param[in]      p3       User thread parameter 3 (Unused).
 * @retval         None
 */
static void app_uart_task(void *p1, void *p2, void *p3)
{
    uint8_t rx_byte = 0;
    int ret;

    (void)p1; (void)p2; (void)p3;

    /* Initialize and start the global watchdog guard right before entering the loop */
    app_watchdog_init();

    LOG_INF("SoC External UART Application Thread Active.");

    while (1)
    {
        ret = UART_Read(&rx_byte, 1, K_FOREVER);

        if (ret > 0)
        {
            /* Pass the address of our explicit static channel instantiation block */
            app_uart_soc_fsm(&g_soc_uart_ctx, rx_byte);

            
            if (rx_byte == 0xFF)
            {
                while(1);
            }
        }

        /* This proves that the main thread scheduler and service routines are actively alive.*/
        app_watchdog_feed();

    }
}


/* System Kernel Thread Declaration Allocation */
K_THREAD_DEFINE(app_uart_thread_id, APP_UART_STACK_SIZE, app_uart_task, NULL, NULL, NULL, APP_UART_PRIORITY, 0, 0);

/**
 * @brief          Initializes the sub-system application logic component.
 * @param          None
 * @retval         int      Returns 0 on successful lifecycle state allocation.
 */
int app_uart_init(void)
{
    /* Initialize the context variable object explicitly */
    g_soc_uart_ctx.current_state = APP_STATE_IDLE;
    g_soc_uart_ctx.packet_idx = 0;
    g_soc_uart_ctx.expected_len = 0;

    app_uart_fsm_init(&g_soc_uart_ctx);
    return 0;
}
