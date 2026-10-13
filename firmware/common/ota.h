#ifndef OTA_H
#define OTA_H
#include <stdint.h>
#define OTA_FLASH_BASE 0x08000000UL
#define OTA_FLASH_SIZE 0x10000UL
#define OTA_PAGE_SIZE 0x400UL
#define OTA_APP_BASE 0x08004000UL
#define OTA_APP_SIZE 0x5C00UL
#define OTA_STAGE_BASE 0x08009C00UL
#define OTA_META0 0x0800F800UL
#define OTA_META1 0x0800FC00UL
#define OTA_RAM_BASE 0x20000000UL
#define OTA_RAM_END 0x20005000UL
#define OTA_MCU_ID 0x103C8UL
#define OTA_EMPTY 0U
#define OTA_VALID 1U
#define OTA_RECEIVING 2U
#define OTA_INSTALLING 3U
#define OTA_STATE_EMPTY OTA_EMPTY
#define OTA_STATE_VALID OTA_VALID
#define OTA_STATE_RECEIVING OTA_RECEIVING
#define OTA_STATE_INSTALLING OTA_INSTALLING
/* All callbacks return 1 on success, 0 on failure. erase takes a page base.
 * write takes an even address; the adapter pads a final odd byte with 0xFF. */
typedef struct {
    void *user;
    int (*read)(void *, uint32_t, uint8_t *, uint32_t);
    int (*erase)(void *, uint32_t);
    int (*write)(void *, uint32_t, const uint8_t *, uint32_t);
} ota_flash_t;
typedef struct {
    uint32_t length, crc, version;
} ota_image_t;
typedef enum {
    OTA_OK = 0,
    OTA_ERR_STATE = 1,
    OTA_ERR_LENGTH = 2,
    OTA_ERR_TARGET = 3,
    OTA_ERR_VECTOR = 4,
    OTA_ERR_CRC = 5,
    OTA_ERR_FLASH = 6,
    OTA_ERR_SESSION = 7,
    OTA_ERR_SEQUENCE = 8,
    OTA_ERR_OFFSET = 9,
    OTA_ERR_CONTENT = 10
} ota_result_t;
typedef struct {
    ota_flash_t flash;
    uint32_t state, request;
    ota_image_t app, candidate;
    uint32_t received, session;
    uint16_t expected_seq;
    uint32_t generation;
    int16_t journal_slot;
    uint8_t usable;
    uint16_t last_seq, last_length;
    uint32_t last_offset;
    uint8_t last_data[128];
} ota_t;
ota_result_t ota_init(ota_t *, const ota_flash_t *);
ota_result_t ota_request_update(ota_t *);
ota_result_t ota_begin(ota_t *, uint32_t, uint32_t, uint32_t,
                       const ota_image_t *);
ota_result_t ota_data(ota_t *, uint32_t, uint16_t, uint32_t, const uint8_t *,
                      uint16_t);
ota_result_t ota_finish(ota_t *, uint32_t);
ota_result_t ota_install(ota_t *);
/* Boot recovery: retain a valid pending source on I/O failure; durably discard
 * a verified invalid source with request=1 so a fresh transfer can begin. */
ota_result_t ota_recover(ota_t *);
ota_result_t ota_cancel(ota_t *, uint32_t);
ota_result_t ota_validate_app(ota_t *);
uint32_t ota_crc32(const uint8_t *, uint32_t);
#endif
