#include "board.h"
#include "app_support.h"
#include "Serial.h"
#include "protocol.h"

static ota_t context;
static ota_parser_t parser;
static ota_frame_t frame, response;
static uint8_t tx[150];
static uint8_t commands[32];
static uint8_t head, tail;
static uint8_t requested;
static uint32_t last_byte;
static uint8_t quarantine;

void Serial_Init(USART_TypeDef *uart, uint32_t baud) {
    (void)uart;
    (void)baud;
    board_init();
}

void App_Init(void) {
    (void)ota_init(&context, board_flash());
    ota_parser_init(&parser);
    requested = 0;
    quarantine = 0;
    head = tail = 0;
    last_byte = board_millis();
}

void App_Poll(void) {
    uint8_t byte;
    uint32_t now = board_millis();
    if (board_rx_fault()) {
        ota_parser_init(&parser);
        quarantine = 1;
        last_byte = now;
    }
    if ((parser.active || quarantine) && (uint32_t)(now - last_byte) > 2000u) {
        ota_parser_init(&parser);
        quarantine = 0;
    }
    while (board_rx_byte(&byte)) {
        uint8_t was_active = parser.active;
        if (byte == 'G')
            quarantine = 1;
        last_byte = board_millis();
        if (ota_parser_feed(&parser, byte, &frame)) {
            /* Frames may still be answered, but binary-burst text stays
             * quarantined until the idle timeout, including damaged bursts. */
            if (ota_handle_frame(&context, &frame, &response, 0)) {
                uint16_t length = ota_frame_encode(&response, tx);
                if (length)
                    board_send(tx, length);
            }
        } else if (!quarantine && !was_active && !parser.active &&
                   (byte == 'S' || byte == 'J' || byte == 'P' || byte == 'R')) {
            uint8_t next = (uint8_t)((head + 1u) & 31u);
            if (next != tail) {
                commands[head] = byte;
                head = next;
            }
        }
    }
    if (!requested && board_key_pressed()) {
        if (ota_request_update(&context) == OTA_OK) {
            requested = 1;
            board_screen("UPDATE REQUEST SAVED", "RESET OR POWER CYCLE",
                         "THEN SEND NEW BIN");
        } else
            board_screen("FLASH STATE ERROR", "REQUEST NOT SAVED",
                         "RESET TO RETRY");
    }
}

int App_UpdateRequested(void) { return requested; }
uint8_t Serial_GetRxFlag(void) { return head != tail; }
uint8_t Serial_GetRxData(void) {
    uint8_t value = 0;
    if (head != tail) {
        value = commands[tail];
        tail = (uint8_t)((tail + 1u) & 31u);
    }
    return value;
}
void Serial_ClearRxFlag(void) { tail = head; }
