#include "stm32f10x.h"
#include "generic.h"
#include "OLED_I2C.h"
#include "Serial.h"


/* 24x24 小鹿帧 1（奔跑） */
const uint8_t Dinosaur1[] =
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0xEE,0xC0,0xC0,0xE0,0xA0,0x18,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xC0,0xC0,0xE0,0xE0,0xE0,0xF0,0xF8,0xFF,0xFE,0xBF,0x1F,0x07,0x07,0x02,0x00,0x00,
    0x00,0x00,0x00,0x04,0x86,0xBF,0x7F,0x07,0x07,0x07,0x07,0x07,0x07,0xBF,0xFF,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

/* 24x24 小鹿帧 2（跳跃） */
const uint8_t Dinosaur2[] =
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0xEE,0xC0,0xC0,0xE0,0xA0,0x18,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xC0,0xC0,0xE0,0xE0,0xE0,0xF0,0xF8,0xFF,0xFE,0xBF,0x1F,0x07,0x07,0x02,0x00,0x00,
    0x00,0x00,0x00,0x04,0x86,0xBF,0x7F,0x07,0x07,0x07,0x07,0x07,0x07,0xBF,0xFF,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

/* 7x6 血量图标 */
const uint8_t blood[] =
{
    0x06,0x0F,0x1F,0x3E,0x1F,0x0F,0x06
};

/* 障碍物 1，7x12 */
const uint8_t obstacle1[] =
{
    0x18,0x30,0xFF,0xFF,0xF0,0xE0,0x7C,
    0x03,0x02,0x0F,0x0F,0x0F,0x01,0x00
};

/* 障碍物 2，7x12 */
const uint8_t obstacle2[] =
{
    0x7F,0xE0,0xC0,0xFC,0x80,0x80,0xE0,
    0x00,0x00,0x0F,0x0F,0x0F,0x05,0x00
};

/* 障碍物 3，9x5 */
const uint8_t obstacle3[] =
{
    0x04,0x1E,0x1E,0x1F,0x1B,0x1F,0x1F,0x16,0x14
};

/* 云朵 1，20x10 */
const uint8_t Cloud1[] =
{
    0x00,0x00,0x3C,0x3C,0x7C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xF8,0xF8,0xF8,0xF8,0x70,0x30,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/* 云朵 2，20x10 */
const uint8_t Cloud2[] =
{
    0x00,0x00,0x38,0x78,0x7C,0xFC,0xFC,0xFC,0xFE,0xFE,0xFE,0xFC,0xFC,0xFC,0x78,0x30,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

#define GAME_READY    0
#define GAME_RUNNING  1
#define GAME_PAUSE    2
#define GAME_OVER     3

static uint8_t GameState = GAME_READY;

/* 小鹿参数 */
static int DinosaurY = 26;
static uint8_t Dinosaur_blood = 3;

/* 障碍物参数 */
static int obstacleX = 130;
static uint8_t obstacle_index = 0;

/* 云朵参数 */
static int16_t CloudX = 150;
static uint8_t Cloud_index = 0;

static uint32_t score = 0;

static uint8_t jump_flag = 0;
static uint8_t back_flag = 0;
static uint8_t thread = 0;

void Game_Reset(void)
{
    GameState = GAME_READY;

    DinosaurY = 26;
    Dinosaur_blood = 3;

    obstacleX = 130;
    obstacle_index = 0;

    CloudX = 150;
    Cloud_index = 0;

    score = 0;

    jump_flag = 0;
    back_flag = 0;
    thread = 0;
}

void Game_Start(void)
{
    DinosaurY = 26;
    Dinosaur_blood = 3;

    obstacleX = 130;
    obstacle_index = 0;

    CloudX = 150;
    Cloud_index = 0;

    score = 0;

    jump_flag = 0;
    back_flag = 0;
    thread = 0;

    GameState = GAME_RUNNING;
}

void Game_HandleSerial(void)
{
    uint8_t cmd;

    if (Serial_GetRxFlag() == 1)
    {
        cmd = Serial_GetRxData();

        if (cmd == 'S')
        {
            if (GameState == GAME_READY || GameState == GAME_OVER)
            {
                Game_Start();
            }
            else if (GameState == GAME_PAUSE)
            {
                GameState = GAME_RUNNING;
            }
        }
        else if (cmd == 'J')
        {
            if (GameState == GAME_RUNNING)
            {
                if (jump_flag == 0 && back_flag == 0)
                {
                    jump_flag = 1;
                }
            }
        }
        else if (cmd == 'P')
        {
            if (GameState == GAME_RUNNING)
            {
                GameState = GAME_PAUSE;
            }
            else if (GameState == GAME_PAUSE)
            {
                GameState = GAME_RUNNING;
            }
        }
        else if (cmd == 'R')
        {
            Game_Reset();
        }
    }
}

void Game_JumpUpdate(void)
{
    if (jump_flag)
    {
        DinosaurY--;
    }

    if (DinosaurY <= 0)
    {
        jump_flag = 0;
        back_flag = 1;
    }

    if (back_flag)
    {
        DinosaurY++;
    }

    if (DinosaurY >= 26)
    {
        DinosaurY = 26;
        back_flag = 0;
    }
}

void Game_ImpactCheck(void)
{
    if (obstacleX <= 22)
    {
        if (DinosaurY >= 14 && Dinosaur_blood > 0 && obstacle_index < 2)
        {
            obstacleX = 150;
            Dinosaur_blood--;
            obstacle_index++;

            if (Dinosaur_blood == 0)
            {
                GameState = GAME_OVER;
            }
        }
        else if (DinosaurY >= 21 && Dinosaur_blood > 0 && obstacle_index == 2)
        {
            obstacleX = 150;
            Dinosaur_blood--;
            obstacle_index++;

            if (Dinosaur_blood == 0)
            {
                GameState = GAME_OVER;
            }
        }
    }
}

void Game_UpdateCloud(void)
{
    CloudX--;

    if (CloudX <= -20)
    {
        Cloud_index++;

        if (Cloud_index >= 2)
        {
            Cloud_index = 0;
        }

        CloudX = 150;
    }
}

void Game_UpdateObstacle(void)
{
    if (obstacleX <= -10)
    {
        obstacleX = 160;
        obstacle_index++;
    }

    if (obstacle_index >= 3)
    {
        obstacle_index = 0;
    }

    obstacleX -= 2;
}

void Game_Update(void)
{
    if (GameState != GAME_RUNNING)
    {
        return;
    }

    thread++;

    if (thread >= 8)
    {
        thread = 0;
    }

    score++;

    Game_UpdateCloud();
    Game_JumpUpdate();
    Game_ImpactCheck();

    if (GameState == GAME_RUNNING)
    {
        Game_UpdateObstacle();
    }
}

void Game_DrawScene(void)
{
    uint8_t i;

    OLED_DrawLine(0, 50, 128, 50);
    OLED_Printf(0, 0, 6, "score:%d", score);

    /* 云朵 */
    if (Cloud_index == 0)
    {
        OLED_ShowImage(CloudX, 10, 20, 10, Cloud1);
    }
    else
    {
        OLED_ShowImage(CloudX, 14, 20, 10, Cloud2);
    }

    /* 小鹿 */
    if (thread <= 4)
    {
                OLED_ShowImage(0, DinosaurY, 24, 24, Dinosaur1);
    }
    else
    {
                OLED_ShowImage(0, DinosaurY, 24, 24, Dinosaur2);
    }

    /* 血量 */
    for (i = 0; i < Dinosaur_blood; i++)
    {
        OLED_ShowImage(120 - i * 8, 6, 7, 6, blood);
    }

    /* 障碍物 */
    if (obstacle_index == 0)
    {
        OLED_ShowImage(obstacleX, 38, 7, 12, obstacle1);
    }
    else if (obstacle_index == 1)
    {
        OLED_ShowImage(obstacleX, 38, 7, 12, obstacle2);
    }
    else
    {
        OLED_ShowImage(obstacleX, 45, 9, 5, obstacle3);
    }
}

void Game_DrawReady(void)
{
    OLED_Printf(18, 8, 6, "DEER RUNNING");
    OLED_Printf(12, 24, 6, "LABVIEW CONTROL");
    OLED_Printf(8, 40, 6, "S START  J JUMP");
    OLED_Printf(14, 52, 6, "P PAUSE  R RESET");
}

void Game_DrawPause(void)
{
    Game_DrawScene();
    OLED_Printf(46, 20, 8, "PAUSE");
}

void Game_DrawOver(void)
{
    Game_DrawScene();
    OLED_Printf(34, 18, 8, "GAME");
    OLED_Printf(34, 34, 8, "OVER");
    OLED_Printf(12, 52, 6, "R RESET S START");
}

void Game_Draw(void)
{
    OLED_Clear();

    if (GameState == GAME_READY)
    {
        Game_DrawReady();
    }
    else if (GameState == GAME_RUNNING)
    {
        Game_DrawScene();
    }
    else if (GameState == GAME_PAUSE)
    {
        Game_DrawPause();
    }
    else if (GameState == GAME_OVER)
    {
        Game_DrawOver();
    }

    OLED_Update();
}

int main(void)
{
    /*
        OLED 接线默认：
        SCL → PB6
        SDA → PB7
    */
    OLED_Init(GPIOB, GPIO_Pin_6, GPIO_Pin_7);
    /*
        LabVIEW 串口控制：
        USB-TTL TX → PA10
        USB-TTL RX → PA9
        波特率：9600
    */
    Serial_Init(USART1, 9600);

    Game_Reset();

    while (1)
    {
        Game_HandleSerial();
        Game_Update();
        Game_Draw();

        Delay_ms(10);
    }
}