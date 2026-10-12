#ifndef APP_SUPPORT_H
#define APP_SUPPORT_H
#include <stdint.h>
void App_Init(void);
void App_Poll(void);
int App_UpdateRequested(void);
extern const uint8_t GameCharacter1[72], GameCharacter2[72];
#ifndef GAME_VERSION
#define GAME_VERSION 1
#endif
#endif
