#include "stdint.h"

//Define ports and pins here
#define LCD_D7_Pin GPIO_PIN_2
#define LCD_D7_GPIO_Port GPIOE
#define LCD_D6_Pin GPIO_PIN_4
#define LCD_D6_GPIO_Port GPIOE
#define LCD_D5_Pin GPIO_PIN_5
#define LCD_D5_GPIO_Port GPIOE
#define LCD_D4_Pin GPIO_PIN_2
#define LCD_D4_GPIO_Port GPIOF
#define LCD_E_Pin GPIO_PIN_8
#define LCD_E_GPIO_Port GPIOF
#define LCD_RW_Pin GPIO_PIN_9
#define LCD_RW_GPIO_Port GPIOF
#define LCD_RS_Pin GPIO_PIN_1
#define LCD_RS_GPIO_Port GPIOG
#define SD_CS_Pin GPIO_PIN_15
#define SD_CS_GPIO_Port GPIOA

typedef void (*Delay_f)(uint16_t);

/**
 * @brief Initializes the display. Note that before you use the display, you need to define
 * ports and pins assigned to control pins of the screen.
 */
void LCD_init(Delay_f delay_function);

/**
 * @brief Clears the screen.
 */
void LCD_clear();

/**
 * @brief Write text to the screen
 * 
 * @param text String to write
 * @param len Length of the string. Must be <= 16.
 */
void LCD_write_text(const char* text, uint32_t len);

/**
 * @brief Set the position of cursor on the LCD
 * 
 * @param x x - position (column). Counted from 1.
 * @param y y - position (row). Counted from 1.
 */
void LCD_position(uint8_t x, uint8_t y);