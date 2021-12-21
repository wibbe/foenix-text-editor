
#include <stdint.h>
#include <string.h>
#include "syscalls.h"


void con_clear_screen(void)
{
	t_con_clear clear;
	clear.screen = 0;
	clear.mode = 2;		// clear entire screen
	sys_chan_ioctrl(0, CON_IOCTRL_CLEAR_SCREEN, (uint8_t*)&clear, sizeof(clear));
}

void con_set_xy(uint16_t x, uint16_t y)
{
	t_con_cursor cursor;
	cursor.screen = 0;
	cursor.x = x;
	cursor.y = y;
	sys_chan_ioctrl(0, CON_IOCTRL_SET_XY, (uint8_t*)&cursor, sizeof(cursor));
}


int main(int argc, char * argv[])
{
	char * msg = "Hello, World\n";

	con_clear_screen();
	con_set_xy(0, 0);

	for (int i = 0; i < argc; ++i)
	{
		sys_chan_write(0, argv[i], strlen(argv[i]));
		sys_chan_write_b(0, '\n');
	}

	sys_chan_write(0, msg, 13);

	return 0;
}