#include "stdint.h"

void LCD_write_nibble(uint8_t nibble, int rs);
void LCD_write_byte(uint8_t byte);
void LCD_write_command(uint8_t command);
void LCD_write_text(char* text, uint32_t len);
void LCD_clear();
void LCD_position(uint8_t x, uint8_t y);
void LCD_write_init_nibble(uint8_t nibble);
void LCD_init();