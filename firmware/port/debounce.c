#include "debounce.h"

void key_debounce_init(key_debounce_t *key) {
    key->at = 0;
    key->waiting = key->consumed = 0;
}

int key_debounce_poll(key_debounce_t *key, uint32_t now, int low, int edge) {
    if (!low) {
        key->waiting = key->consumed = 0;
        return 0;
    }
    if (key->consumed)
        return 0;
    /* Every later falling edge restarts the stable-low interval. */
    if (edge) {
        key->at = now;
        key->waiting = 1;
    }
    if (key->waiting && (uint32_t)(now - key->at) >= 30u) {
        key->waiting = 0;
        key->consumed = 1;
        return 1;
    }
    return 0;
}
