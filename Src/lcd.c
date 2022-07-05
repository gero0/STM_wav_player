#include "lcd.h"
#include <main.h>
#include <stm32h7xx_hal.h>

void LCD_write_nibble(uint8_t nibble, int rs)
{
    HAL_GPIO_WritePin(LCD_RS_GPIO_Port, LCD_RS_Pin, rs);
    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, 1);

    HAL_GPIO_WritePin(LCD_D4_GPIO_Port, LCD_D4_Pin, nibble & (1 << 0));
    HAL_GPIO_WritePin(LCD_D5_GPIO_Port, LCD_D5_Pin, nibble & (1 << 1));
    HAL_GPIO_WritePin(LCD_D6_GPIO_Port, LCD_D6_Pin, nibble & (1 << 2));
    HAL_GPIO_WritePin(LCD_D7_GPIO_Port, LCD_D7_Pin, nibble & (1 << 3));

    HAL_GPIO_WritePin(LCD_E_GPIO_Port, LCD_E_Pin, 0);
}

void LCD_write_byte(uint8_t byte)
{
    uint8_t low_nibble = byte & 0xF;
    uint8_t high_nibble = (byte >> 4) & 0xF;
    LCD_write_nibble(high_nibble, 1);
    LCD_write_nibble(low_nibble, 1);

    delay_us(200);
}

void LCD_write_command(uint8_t command)
{
    uint8_t low_nibble = command & 0xF;
    uint8_t high_nibble = (command >> 4) & 0xF;
    LCD_write_nibble(high_nibble, 0);
    LCD_write_nibble(low_nibble, 0);

    delay_us(800);
}

void LCD_write_text(char* text, uint32_t len)
{
    for (int i = 0; i < len; i++) {
        LCD_write_byte(text[i]);
    }
}

void LCD_clear()
{
    LCD_write_command(1);
}

void LCD_position(uint8_t x, uint8_t y)
{
    int temp = 127 + x;
    if (y == 2)
        temp = temp + 64;
    LCD_write_command(temp);
}

void LCD_write_init_nibble(uint8_t nibble)
{
    LCD_write_nibble((nibble >> 4) & 0x0f, 0);
}

void LCD_init()
{
    LCD_write_init_nibble(0x20); // Wake-Up Sequence
    delay_us(5000);
    LCD_write_init_nibble(0x20);
    delay_us(5000);
    LCD_write_init_nibble(0x20);
    delay_us(5000);

    LCD_write_command(0x28); // 4-bits, 2 lines, 5x7 font
    delay_us(5000);
    LCD_write_command(0x0C); // Display ON, No cursors
    delay_us(5000);
    LCD_write_command(0x06); // Entry mode- Auto-increment, No Display shifting
    delay_us(5000);
    LCD_write_command(0x01);
    delay_us(5000);
}