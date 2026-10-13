#include <stdio.h>
#include <string.h>
#include "board.h"
#include "Serial.h"
#include "app_support.h"
#include "protocol.h"
static uint32_t clock_ms;
static uint8_t input[512], memory[65536];
static uint16_t count, pos;
static int key, fault, screens;
static uint8_t sent[150];
static uint16_t sent_length;
static int read_flash(void *u, uint32_t a, uint8_t *d, uint32_t n) {
    (void)u;
    if (a < OTA_FLASH_BASE || n > OTA_FLASH_SIZE ||
        a > OTA_FLASH_BASE + OTA_FLASH_SIZE - n)
        return 0;
    memcpy(d, memory + a - OTA_FLASH_BASE, n);
    return 1;
}
static int erase_flash(void *u, uint32_t a) {
    (void)u;
    if (a < OTA_APP_BASE || a > OTA_FLASH_BASE + OTA_FLASH_SIZE - 1024 ||
        (a & 1023))
        return 0;
    memset(memory + a - OTA_FLASH_BASE, 255, 1024);
    return 1;
}
static int write_flash(void *u, uint32_t a, const uint8_t *d, uint32_t n) {
    uint32_t i;
    (void)u;
    if (a < OTA_APP_BASE || n > OTA_FLASH_SIZE ||
        a > OTA_FLASH_BASE + OTA_FLASH_SIZE - n)
        return 0;
    for (i = 0; i < n; i++) {
        if (memory[a - OTA_FLASH_BASE + i] != 255)
            return 0;
        memory[a - OTA_FLASH_BASE + i] = d[i];
    }
    return 1;
}
static const ota_flash_t f = {0, read_flash, erase_flash, write_flash};
void board_init(void) {}
uint32_t board_millis(void) { return clock_ms; }
int board_key_pressed(void) {
    int value = key;
    key = 0;
    return value;
}
int board_rx_fault(void) {
    int value = fault;
    fault = 0;
    return value;
}
int board_rx_byte(uint8_t *b) {
    if (pos == count)
        return 0;
    *b = input[pos++];
    return 1;
}
void board_send(const uint8_t *b, uint16_t n) {
    memcpy(sent, b, n);
    sent_length = n;
}
const ota_flash_t *board_flash(void) { return &f; }
void board_screen(const char *a, const char *b, const char *c) {
    (void)a;
    (void)b;
    (void)c;
    screens++;
}
static void feed(const uint8_t *p, uint16_t n) {
    memcpy(input, p, n);
    count = n;
    pos = 0;
    App_Poll();
}
#define CHECK(c)                                                               \
    do {                                                                       \
        if (!(c)) {                                                            \
            printf("app port FAIL line%d: %s\n", __LINE__, #c);                \
            return 1;                                                          \
        }                                                                      \
    } while (0)
int main(void) {
    ota_frame_t frame;
    uint8_t encoded[150], bad[40];
    uint16_t n;
    ota_t cold;
    memset(memory, 255, sizeof(memory));
    App_Init();
    feed((const uint8_t *)"SJPR", 4);
    CHECK(Serial_GetRxFlag());
    CHECK(Serial_GetRxData() == 'S');
    CHECK(Serial_GetRxData() == 'J');
    CHECK(Serial_GetRxData() == 'P');
    CHECK(Serial_GetRxData() == 'R');
    CHECK(!Serial_GetRxFlag());
    memset(&frame, 0, sizeof(frame));
    frame.type = OTA_BEGIN;
    frame.length = 20;
    memcpy(frame.payload, "SJPRSJPR", 8);
    n = ota_frame_encode(&frame, encoded);
    feed(encoded, n);
    CHECK(!Serial_GetRxFlag());
    encoded[n - 1] ^= 1;
    feed(encoded, n);
    CHECK(!Serial_GetRxFlag());
    clock_ms += 2001;
    App_Poll();
    memcpy(bad, "GBOT\1\2\0\0\0\0\0\0\x85\0SJPR", 18);
    feed(bad, 18);
    CHECK(!Serial_GetRxFlag());
    feed((const uint8_t *)"SJPR", 4);
    CHECK(!Serial_GetRxFlag());
    clock_ms += 2001;
    App_Poll();
    feed((const uint8_t *)"S", 1);
    CHECK(Serial_GetRxData() == 'S');
    /* A valid QUERY must not release text from the same damaged binary burst.
     */
    memset(&frame, 0, sizeof(frame));
    frame.type = OTA_QUERY;
    frame.session = 42;
    n = ota_frame_encode(&frame, encoded);
    memcpy(input, "GBOT\1\2\0\0\0\0\0\0\x85\0", 14);
    memcpy(input + 14, encoded, n);
    memcpy(input + 14 + n, "SJPR", 4);
    count = (uint16_t)(18 + n);
    pos = 0;
    sent_length = 0;
    App_Poll();
    CHECK(!Serial_GetRxFlag());
    CHECK(sent_length == 46 && sent[5] == OTA_ACK && sent[6] == 42);
    clock_ms += 2001;
    App_Poll();
    feed((const uint8_t *)"S", 1);
    CHECK(Serial_GetRxFlag() && Serial_GetRxData() == 'S' &&
          !Serial_GetRxFlag());
    fault = 1;
    App_Poll();
    feed((const uint8_t *)"SJPR", 4);
    CHECK(!Serial_GetRxFlag());
    clock_ms += 2001;
    App_Poll();
    key = 1;
    App_Poll();
    CHECK(App_UpdateRequested());
    CHECK(screens == 1);
    CHECK(ota_init(&cold, &f) == OTA_OK && cold.request == 1);
    puts("app port: text/binary isolation, fault quarantine and durable key "
         "request PASS");
    return 0;
}
