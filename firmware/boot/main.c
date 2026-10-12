#include "board.h"
#include "protocol.h"
#include "OLED_I2C.h"

static ota_t context;
static ota_parser_t parser;
static ota_frame_t frame, response;
static uint8_t tx[150];

int main(void)
{
    uint8_t byte;
    uint8_t updater = 0;
    uint32_t start, last_byte, done_at = 0;
    board_init();
    OLED_Init(GPIOB, GPIO_Pin_6, GPIO_Pin_7);
    board_screen("GAME BOOTLOADER", "KEY: UPDATE (3 SEC)", "C8T6 / LORA 9600");
    if (ota_init(&context, board_flash()) != OTA_OK) updater = 1;
    if (context.state == OTA_INSTALLING) {
        board_screen("RECOVER INSTALL", "KEEP POWER ON", "VERIFYING STAGE");
        if (ota_install(&context) != OTA_OK) updater = 1;
    }
    if (context.request || ota_validate_app(&context) != OTA_OK) updater = 1;
    start = board_millis();
    while (!updater && (uint32_t)(board_millis() - start) < 3000u) {
        if (board_key_pressed()) {
            if (ota_request_update(&context) != OTA_OK)
                board_screen("STATE WRITE ERROR", "STAY IN BOOTLOADER", "SEND BIN TO RECOVER");
            updater = 1;
        }
    }
    if (!updater) board_jump_to_app();
    board_screen("WAITING FOR BIN", "OPEN PYTHON UPDATER", "KEY / LORA READY");
    ota_parser_init(&parser);
    last_byte = board_millis();
    while (1) {
        uint32_t now = board_millis();
        if (board_rx_fault()) ota_parser_init(&parser);
        if (parser.active && (uint32_t)(now - last_byte) > 2000u) ota_parser_init(&parser);
        while (board_rx_byte(&byte)) {
            last_byte = board_millis();
            if (ota_parser_feed(&parser, byte, &frame)) {
                if (frame.type == OTA_COMMIT) board_screen("VERIFY AND INSTALL", "KEEP POWER ON", "CHECKING CRC32");
                if (ota_handle_frame(&context, &frame, &response, 1)) {
                    uint16_t length = ota_frame_encode(&response, tx);
                    if (length) board_send(tx, length);
                    if ((response.type == OTA_DONE || (frame.type == OTA_CANCEL && response.type == OTA_ACK)) &&
                        ota_validate_app(&context) == OTA_OK) {
                        done_at = board_millis();
                        if (!done_at) done_at = 1;
                        board_screen("INSTALL / CANCEL OK", "START GAME IN 10 SEC", "CRC32 VERIFIED");
                    } else if (response.type == OTA_NACK) {
                        board_screen("UPDATE ERROR", "CHECK PC ERROR CODE", "RETRY OR RESEND BIN");
                    } else if (frame.type == OTA_BEGIN && response.type == OTA_ACK) {
                        done_at = 0;
                        board_screen("RECEIVING NEW BIN", "KEEP POWER ON", "OLD APP PRESERVED");
                    }
                }
            }
        }
        if (done_at && (uint32_t)(board_millis() - done_at) >= 10000u) board_reset();
    }
}
