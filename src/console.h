
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdint.h>
#include "types.h"

#define CON_KEY_UP			0xA0
#define CON_KEY_DOWN		0xA1
#define CON_KEY_LEFT		0xA2
#define CON_KEY_RIGHT		0xA3
#define CON_KEY_HOME 		0xA4
#define CON_KEY_INSERT		0xA5
#define CON_KEY_DELETE		0xA6
#define CON_KEY_END			0xA7
#define CON_KEY_PAGE_UP		0xA8
#define CON_KEY_PAGE_DOWN	0xA9
#define CON_KEY_F1			0xB0
#define CON_KEY_F2			0xB1
#define CON_KEY_F3			0xB2
#define CON_KEY_F4			0xB3
#define CON_KEY_F5			0xB4
#define CON_KEY_F6			0xB5
#define CON_KEY_F7			0xB6
#define CON_KEY_F8			0xB7
#define CON_KEY_F9			0xB8
#define CON_KEY_F10			0xB9
#define CON_KEY_F11			0xBA
#define CON_KEY_F12			0xBB

void con_setup(void);
void con_teardown(void);

void con_clear_line(void);
void con_clear_screen(void);
void con_set_xy(uint16_t x, uint16_t y);
void con_set_color(int16_t foreground, int16_t background);
void con_get_size(int16_t *x, int16_t *y);
uint8_t con_get_key(void);

#endif