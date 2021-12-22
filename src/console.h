
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include <stdbool.h>
#include "types.h"


#define CON_COLOR_BLACK         0
#define CON_COLOR_RED           1
#define CON_COLOR_GREEN         2
#define CON_COLOR_YELLOW        3
#define CON_COLOR_BLUE          4
#define CON_COLOR_ORANGE        5
#define CON_COLOR_CYAN          6
#define CON_COLOR_GREY          7
#define CON_COLOR_DARK_GREY     8
#define CON_COLOR_BRIGHT_RED    9
#define CON_COLOR_BRIGHT_GREEN  10
#define CON_COLOR_BRIGHT_YELLOW 11
#define CON_COLOR_BRIGHT_BLUE   12
#define CON_COLOR_BRIGHT_ORANGE 13
#define CON_COLOR_BRIGHT_CYAN   14
#define CON_COLOR_WHITE         15

#define CON_KEY_ESC             0x1B
#define CON_KEY_BACKSPACE       0x08
#define CON_KEY_ENTER           0x0D
#define CON_KEY_UP              0xA0
#define CON_KEY_DOWN            0xA1
#define CON_KEY_LEFT            0xA2
#define CON_KEY_RIGHT           0xA3
#define CON_KEY_HOME            0xA4
#define CON_KEY_INSERT          0xA5
#define CON_KEY_DELETE          0xA6
#define CON_KEY_END             0xA7
#define CON_KEY_PAGE_UP         0xA8
#define CON_KEY_PAGE_DOWN       0xA9
#define CON_KEY_F1              0xB0
#define CON_KEY_F2              0xB1
#define CON_KEY_F3              0xB2
#define CON_KEY_F4              0xB3
#define CON_KEY_F5              0xB4
#define CON_KEY_F6              0xB5
#define CON_KEY_F7              0xB6
#define CON_KEY_F8              0xB7
#define CON_KEY_F9              0xB8
#define CON_KEY_F10             0xB9
#define CON_KEY_F11             0xBA
#define CON_KEY_F12             0xBB
#define CON_KEY_CTRL_Q          0x11
#define CON_KEY_CTRL_S          0x13

#define CON_CHAR_ESC            '\x1B'  /* Escape character */
#define CON_CHAR_TAB            '\t'    /* Vertical tab */
#define CON_CHAR_CR             '\x0D'  /* Carriage return */
#define CON_CHAR_NL             '\x0A'  /* Linefeed */
#define CON_CHAR_BS             '\b'    /* Backspace */


void con_setup(void);
void con_teardown(void);

void con_clear_line(void);
void con_clear_screen(void);

void con_set_cursor(int16_t color, uint8_t character, int16_t rate, bool enable);
void con_set_border(bool visible, int16_t width, int16_t height, uint32_t color);
void con_set_xy(uint16_t x, uint16_t y);
void con_set_color(int16_t foreground, int16_t background);
void con_get_size(int16_t *x, int16_t *y);

void con_out(uint8_t ch);
void con_out_raw(uint8_t ch);
void con_newline(void);
void con_write(uint8_t * buffer, uint16_t size);

uint8_t con_get_key(void);

#endif