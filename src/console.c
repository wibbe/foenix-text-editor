
#include "console.h"
#include "syscalls.h"



void con_setup(void)
{
	con_clear_screen();
	con_set_xy(0, 0);
	//con_set_color(CON_COLOR_BLUE, CON_COLOR_GREY);

	sys_chan_ioctrl(0, CON_IOCTRL_ANSI_OFF, 0, 0);
	sys_chan_ioctrl(0, CON_IOCTRL_ECHO_OFF, 0, 0);
}

void con_teardown(void)
{
	sys_chan_ioctrl(0, CON_IOCTRL_ANSI_ON, 0, 0);
	sys_chan_ioctrl(0, CON_IOCTRL_ECHO_ON, 0, 0);
	con_set_color(CON_COLOR_GREY, CON_COLOR_BLUE);

	con_clear_screen();
	con_set_xy(0, 0);
}

void con_clear_screen(void)
{
	static t_con_clear clear;
	clear.screen = 0;
	clear.mode = 2;		// clear entire screen
	sys_chan_ioctrl(0, CON_IOCTRL_CLEAR_SCREEN, (uint8_t*)&clear, sizeof(clear));
}

void con_clear_line(void)
{
	static t_con_clear clear;
	clear.screen = 0;
	clear.mode = 2;
	sys_chan_ioctrl(0, CON_IOCTRL_CLEAR_LINE, (uint8_t*)&clear, sizeof(clear));
}

void con_set_xy(uint16_t x, uint16_t y)
{
	static t_con_cursor cursor;
	cursor.screen = 0;
	cursor.x = x;
	cursor.y = y;
	sys_chan_ioctrl(0, CON_IOCTRL_SET_XY, (uint8_t*)&cursor, sizeof(cursor));
}

void con_set_color(int16_t foreground, int16_t background)
{
	static t_con_color color;
	color.screen = 0;
	color.foreground = foreground;
	color.background = background;
	sys_chan_ioctrl(0, CON_IOCTRL_SET_COLOR, (uint8_t*)&color, sizeof(t_con_color));
}

void con_get_size(int16_t *x, int16_t *y)
{
	static t_con_size size;
	size.screen = 0;
	if (sys_chan_ioctrl(0, CON_IOCTRL_GET_SIZE, (uint8_t*)&size, sizeof(t_con_size)) >= 0)
	{
		*x = size.x;
		*y = size.y;
	}
	else
	{
		*x = -1;
		*y = -1;
	}
}

uint8_t con_get_key(void)
{
	int16_t key = 0;

	while (key == 0)
	{
		key = sys_chan_read_b(0);

		// Escape sequence start?
		if (key == 0x1B)
		{
			key = sys_chan_read_b(0);
			if (key == '[')
			{
				key = sys_chan_read_b(0);

				switch (key)
				{
					case 'A':	// Up
						return CON_KEY_UP;

					case 'B':	// Left
						return CON_KEY_LEFT;

					case 'C': 	// Right
						return CON_KEY_RIGHT;

					case 'D': 	// Down
						return CON_KEY_DOWN;

					case '1':
						{
							key = sys_chan_read_b(0);
							switch (key)
							{
								case '~':
									return CON_KEY_HOME;
								case '1':
									key = sys_chan_read_b(0);	// read last ~
									return CON_KEY_F1;
								case '2':
									key = sys_chan_read_b(0);
									return CON_KEY_F2;
								case '3':
									key = sys_chan_read_b(0);
									return CON_KEY_F3;
								case '4':
									key = sys_chan_read_b(0);
									return CON_KEY_F4;
								case '5':
									key = sys_chan_read_b(0);
									return CON_KEY_F5;
								case '7':
									key = sys_chan_read_b(0);
									return CON_KEY_F6;
								case '8':
									key = sys_chan_read_b(0);
									return CON_KEY_F8;
								case '9':
									key = sys_chan_read_b(0);
									return CON_KEY_F9;
								default:
									key = 0;
									break;
							}
						}
						break;

					case '2':
						{
							key = sys_chan_read_b(0);
							switch (key)
							{
								case '~':
									return CON_KEY_INSERT;
								case '0':
									key = sys_chan_read_b(0);	// read last ~
									return CON_KEY_F9;
								case '1':
									key = sys_chan_read_b(0);	// read last ~
									return CON_KEY_F10;
								case '3':
									key = sys_chan_read_b(0);
									return CON_KEY_F11;
								case '4':
									key = sys_chan_read_b(0);
									return CON_KEY_F12;
								default:
									key = 0;
									break;
							}
						}
						break;

					case '3':
						key = sys_chan_read_b(0);
						return CON_KEY_DELETE;

					case '4':
						key = sys_chan_read_b(0);
						return CON_KEY_END;

					case '5':
						key = sys_chan_read_b(0);
						return CON_KEY_PAGE_UP;

					case '6':
						key = sys_chan_read_b(0);
						return CON_KEY_PAGE_DOWN;

					default:
						key = 0;
						break;
				}
			}
			else if (key == 0x1B)
			{
				return key;
			}
			else
			{
				// this should not happen
				key = 0;
			}
		}
	}


	return (uint8_t)key;
}