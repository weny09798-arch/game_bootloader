#ifndef OTA_PROTOCOL_H
#define OTA_PROTOCOL_H
#include "ota.h"
#define OTA_QUERY 1U
#define OTA_BEGIN 2U
#define OTA_DATA 3U
#define OTA_COMMIT 4U
#define OTA_CANCEL 5U
#define OTA_ACK 6U
#define OTA_NACK 7U
#define OTA_DONE 8U
#define OTA_INFO_APPLICATION 0x80000000UL
typedef struct {
    uint8_t type;
    uint32_t session;
    uint16_t sequence, length;
    uint8_t payload[132];
} ota_frame_t;
typedef struct {
    uint8_t buffer[150];
    uint16_t used;
    uint8_t active;
} ota_parser_t;
void ota_parser_init(ota_parser_t *);
int ota_parser_feed(ota_parser_t *, uint8_t, ota_frame_t *);
uint16_t ota_frame_encode(const ota_frame_t *, uint8_t *);
int ota_handle_frame(ota_t *, const ota_frame_t *, ota_frame_t *, int);
#endif
