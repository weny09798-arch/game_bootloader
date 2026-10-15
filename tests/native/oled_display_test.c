/* Real graphics/game code; only physical GPIO/I2C writes are substituted. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "OLED_I2C.h"
#include "app_support.h"

extern uint8_t OLED_DisplayBuf[8][128];
void Game_Reset(void);
void Game_Start(void);
void Game_Draw(void);

static uint8_t ram[8][128];
static unsigned page, column, column_start, argument, dirty_when_enabled;
static unsigned failures;

/* The firmware main loop and physical STM32 transport must never run here. */
static void unexpected_hardware(void) { abort(); }
void RCC_APB2PeriphClockCmd(uint32_t clock, int enable) {
    (void)clock; (void)enable; unexpected_hardware();
}
void GPIO_Init(GPIO_TypeDef *gpio, GPIO_InitTypeDef *init) {
    (void)gpio; (void)init; unexpected_hardware();
}
void Serial_Init(USART_TypeDef *uart, uint32_t baud) {
    (void)uart; (void)baud; unexpected_hardware();
}
uint8_t Serial_GetRxFlag(void) { unexpected_hardware(); return 0; }
uint8_t Serial_GetRxData(void) { unexpected_hardware(); return 0; }
void App_Init(void) { unexpected_hardware(); }
void App_Poll(void) { unexpected_hardware(); }
int App_UpdateRequested(void) { unexpected_hardware(); return 0; }
void Delay_ms(uint32_t ms) { (void)ms; unexpected_hardware(); }

#define CHECK(test, message) do { if (!(test)) { \
    fprintf(stderr, "FAIL: %s\n", message); ++failures; } } while (0)

void OLED_GPIO_Init(GPIO_TypeDef *gpio, uint16_t scl, uint16_t sda) {
    (void)gpio; (void)scl; (void)sda;
}

void OLED_WriteCommand(uint8_t command) {
    unsigned p, x;
    if (argument) { argument = 0; return; }
    if (command == 0xD5 || command == 0xA8 || command == 0xD3 ||
        command == 0xDA || command == 0x81 || command == 0xD9 ||
        command == 0xDB || command == 0x8D || command == 0x20) {
        argument = 1;
    } else if ((command & 0xF8) == 0xB0) {
        page = command & 7;
    } else if ((command & 0xF0) == 0x10) {
        column = (column & 15u) | ((command & 15u) << 4);
        column_start = column;
    } else if ((command & 0xF0) == 0) {
        column = (column & 0xF0u) | (command & 15u);
        column_start = column;
    } else if (command == 0xAF) {
        for (p = 0; p < 8; ++p)
            for (x = 0; x < 128; ++x)
                if (ram[p][x]) ++dirty_when_enabled;
    }
}

void OLED_WriteData(uint8_t *data, uint8_t count) {
    unsigned i;
    for (i = 0; i < count; ++i) {
        ram[page][column] = data[i];
        if (++column >= 128) column = column_start;
    }
}

static void cold_panel(void) {
    page = column = column_start = argument = dirty_when_enabled = 0;
    /* Power-on display RAM is unspecified, including edge columns. */
    memset(ram, 0xA5, sizeof(ram));
    OLED_Init(GPIOB, GPIO_Pin_6, GPIO_Pin_7);
}

static void check_clean_panel(void) {
    unsigned p, x, dirty = 0;
    cold_panel();
    for (p = 0; p < 8; ++p)
        for (x = 0; x < 128; ++x)
            if (ram[p][x]) ++dirty;
    CHECK(dirty == 0, "initialization leaves dirty controller edge columns");
    CHECK(dirty_when_enabled == 0, "display enabled before its RAM is cleared");
}

static int text_matches(unsigned x, unsigned y, const char *text) {
    unsigned i, dx, dy;
    for (i = 0; text[i]; ++i) {
        for (dx = 0; dx < 6; ++dx) {
            uint8_t glyph = OLED_F6x8[(unsigned char)text[i] - ' '][dx];
            for (dy = 0; dy < 8; ++dy) {
                unsigned px = x + i * 6 + dx, py = y + dy;
                unsigned actual = (OLED_DisplayBuf[py / 8][px] >> (py % 8)) & 1;
                if (actual != ((glyph >> dy) & 1u)) return 0;
            }
        }
    }
    return 1;
}

static void check_game_layout(void) {
    static const uint8_t heart[7] = {6, 15, 31, 62, 31, 15, 6};
    unsigned i;
    const char *version = GAME_VERSION == 1 ? "V1" : "V2";
    cold_panel();
    Game_Reset();
    Game_Draw();
    CHECK(text_matches(114, 0, version), "ready version missing from top-right");
    CHECK(text_matches(14, 52, "P PAUSE  R RESET"), "version overwrites ready controls");
    /* A 128-column SSD1306 must map logical column zero to hardware zero. */
    CHECK(memcmp(ram, OLED_DisplayBuf, sizeof(ram)) == 0, "SSD1306 framebuffer shifted by hardcoded column offset");
    Game_Start();
    Game_Draw();
    CHECK(text_matches(114, 0, version), "running version missing from top-right");
    for (i = 0; i < 7; ++i)
        CHECK(OLED_DisplayBuf[7][120 + i] == heart[i], "life icon overlaps version or is clipped");
}

int main(int argc, char **argv) {
    unsigned x, y;
    check_clean_panel();
    check_game_layout();
    if (failures) return 1;
    if (argc > 1) {
        /* Optional PBM snapshot of the actual framebuffer for visual review. */
        Game_Reset();
        if (strcmp(argv[1], "running") == 0) Game_Start();
        Game_Draw();
        printf("P1\n128 64\n");
        for (y = 0; y < 64; ++y) {
            for (x = 0; x < 128; ++x)
                printf("%u ", (OLED_DisplayBuf[y / 8][x] >> (y % 8)) & 1u);
            printf("\n");
        }
    } else {
        printf("OLED V%d: SSD1306 cold RAM clear and real game layout PASS\n", GAME_VERSION);
    }
    return 0;
}
