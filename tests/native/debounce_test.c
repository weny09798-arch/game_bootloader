#include <stdio.h>
#include "debounce.h"

static int checks;
#define CHECK(c)                                                               \
    do {                                                                       \
        ++checks;                                                              \
        if (!(c)) {                                                            \
            printf("debounce FAIL line %d: %s\n", __LINE__, #c);               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    key_debounce_t key;
    key_debounce_init(&key);
    CHECK(!key_debounce_poll(&key, 0, 1, 1));
    CHECK(!key_debounce_poll(&key, 25, 1, 1));
    CHECK(!key_debounce_poll(&key, 35, 1, 0));
    CHECK(!key_debounce_poll(&key, 54, 1, 0));
    CHECK(key_debounce_poll(&key, 55, 1, 0));
    CHECK(!key_debounce_poll(&key, 90, 1, 1));
    CHECK(!key_debounce_poll(&key, 120, 1, 0));
    CHECK(!key_debounce_poll(&key, 121, 0, 0));
    CHECK(!key_debounce_poll(&key, 122, 1, 1));
    CHECK(!key_debounce_poll(&key, 151, 1, 0));
    CHECK(key_debounce_poll(&key, 152, 1, 0));
    CHECK(!key_debounce_poll(&key, 180, 0, 1));
    CHECK(!key_debounce_poll(&key, 200, 1, 0));
    CHECK(!key_debounce_poll(&key, 240, 1, 0));

    key_debounce_init(&key);
    CHECK(!key_debounce_poll(&key, 0xFFFFFFF0u, 1, 1));
    CHECK(!key_debounce_poll(&key, 0x0000000Du, 1, 0));
    CHECK(key_debounce_poll(&key, 0x0000000Eu, 1, 0));
    CHECK(!key_debounce_poll(&key, 0x00000050u, 1, 0));
    CHECK(!key_debounce_poll(&key, 0x00000051u, 0, 0));
    CHECK(!key_debounce_poll(&key, 0xFFFFFFF0u, 1, 1));
    CHECK(!key_debounce_poll(&key, 5, 1, 1));
    CHECK(!key_debounce_poll(&key, 34, 1, 0));
    CHECK(key_debounce_poll(&key, 35, 1, 0));
    printf("debounce: %d checks PASS\n", checks);
    return 0;
}
