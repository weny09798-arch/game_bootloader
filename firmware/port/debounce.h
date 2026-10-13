#ifndef GAME_DEBOUNCE_H
#define GAME_DEBOUNCE_H
#include <stdint.h>

typedef struct {
    uint32_t at;
    uint8_t waiting, consumed;
} key_debounce_t;

void key_debounce_init(key_debounce_t *key);
/* low is the sampled GPIO level; edge is an atomically consumed falling event.
 */
int key_debounce_poll(key_debounce_t *key, uint32_t now, int low, int edge);
#endif
