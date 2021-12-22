
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "syscalls.h"
#include "console.h"
#include "mem.h"


typedef bool (*command_t)(uint8_t ch);

typedef struct line_t {
	struct line_t *prev;
	struct line_t *next;
	uint8_t len;
	uint8_t cap;
	uint8_t data[1];
} line_t;

typedef struct line_cache_t {
	line_t *first;
	uint8_t size;
} line_cache_t;

typedef struct location_t {
	line_t *line;
	uint8_t offset;
} location_t;


static int16_t _width;
static int16_t _height;

static location_t _scroll;
static location_t _cursor;
static uint16_t _cursor_x = 0;
static uint16_t _cursor_y = 0;

static line_t *_document_first_line = 0;

static command_t _basic_commands[256];
static command_t * _current_commands;

static line_t _line_cache_8 = {0};
static line_t _line_cache_32 = {0};
static line_t _line_cache_64 = {0};
static line_t _line_cache_128 = {0};



/** Utilities **/


static void error(const char * msg)
{
	con_teardown();
	sys_chan_write(0, (char*)msg, strlen(msg));
	sys_exit(1);
}




/** Line and Document **/

static line_t *get_line_cache(uint16_t *size)
{
	if (*size <= 8)
	{
		*size = 8;
		return &_line_cache_8;
	}
	else if (*size <= 32)
	{
		*size = 32;
		return &_line_cache_32;
	}
	else if (*size <= 64)
	{
		*size = 64;
		return &_line_cache_64;
	}
	else if (*size <= 128)
	{
		*size = 128;
		return &_line_cache_128;
	}
	else
	{
		error("Requested line to large");
	}
}

static line_t *new_line_from_cache(line_t *cache, uint16_t size)
{
	line_t *line = 0;
	if (cache->next == 0)
	{
		line = (line_t*)mem_alloc(sizeof(line_t) + size - 1);
		if (line == 0)
			error("Out of memory, could not allocate line");

		line->cap = size;
	}
	else
	{
		line = cache->next;
		cache->next = line->next;
	}

	line->len = 0;
	line->prev = 0;
	line->next = 0;

	return line;
}

static line_t *alloc_line(uint16_t size)
{
	line_t *cache = get_line_cache(&size);
	return new_line_from_cache(cache, size);
}

static void free_line(line_t *line)
{
	uint16_t size = line->cap;
	line_t *cache = get_line_cache(&size);
	line->next = cache->next;
	cache->next = line;
}

static line_t *grow_line(line_t *line)
{
	uint16_t size = line->cap + 1;
	line_t *cache = get_line_cache(&size);
	line_t *new_line = new_line_from_cache(cache, size);

	new_line->len = line->len;
	memcpy(new_line->data, line->data, line->len);


	if (line->prev != 0)
	{
		new_line->prev = line->prev;
		line->prev->next = new_line;
	}
	if (line->next != 0)
	{
		new_line->next = line->next;
		line->next->prev = new_line;
	}


	free_line(line);
	return new_line;
}


static void free_document(void)
{
	if (_document_first_line != 0)
	{
		line_t *it = _document_first_line;
		while (it != 0)
		{
			line_t *to_free = it;
			it = it->next;

			free_line(to_free);
		}
	}

	_document_first_line = 0;
}

static void new_document(void)
{
	free_document();

	_document_first_line = alloc_line(8);
	_scroll.line = _document_first_line;
	_scroll.offset = 0;
	_cursor.line = _document_first_line;
	_cursor.offset = 0;
}



/** Painting **/

static uint16_t get_current_line_number(void)
{
	line_t * line = _scroll.line;
	line_t * current = _cursor.line;
	uint16_t line_number = 0;

	while (line != 0 && line != current)
	{
		line = line->next;
		++line_number;
	}

	return line_number;
}

static void display_statusbar(char * msg)
{
	con_set_xy(0, _height - 1);
	con_set_color(CON_COLOR_BLUE, CON_COLOR_GREY);
	con_clear_line();

	int len = strlen(msg);
	sys_chan_write_b(0, ' ');
	sys_chan_write(0, msg, len);

	con_set_color(CON_COLOR_GREY, CON_COLOR_BLUE);
	con_set_xy(_cursor_x, _cursor_y);
}

static uint8_t redisplay_line(line_t *line)
{
	con_clear_line();

	uint8_t pos = 0;
	for (uint8_t i = 0; i < line->len; ++i)
	{
		uint8_t ch = line->data[i];

		if (ch == '\t')
		{
			uint8_t tab_count = pos / 2;
			uint8_t new_pos = (tab_count + 1) * 2;

			while (pos < new_pos)
			{
				++pos;
				sys_chan_write_b(0, ' ');
			}
		}
		else
		{
			++pos;
			sys_chan_write_b(0, ch);
		}
	}
	return pos;
}

static void redisplay_current_line(void)
{
	uint16_t line_number = get_current_line_number();
	con_set_xy(0, line_number);

	_cursor_x = redisplay_line(_cursor.line);
	_cursor_y = line_number;
	con_set_xy(_cursor_x, _cursor_y);
}

static void redisplay_all(void)
{
	con_set_xy(0, 0);

	uint16_t line_number = 0;
	uint8_t x = _cursor.offset;
	line_t *it = _scroll.line;

	while (it != 0 && line_number < (_height - 1))
	{
		if (it == _cursor.line)
			x = redisplay_line(it);
		else
			redisplay_line(it);

		sys_chan_write_b(0, '\n');
		
		line_number++;
		it = it->next;
	}

	_cursor_x = x;
	_cursor_y = line_number;
	con_set_xy(_cursor_x, _cursor_y);
}

static void redisplay_current_line_down(void)
{
	uint16_t line_number = get_current_line_number();
	con_set_xy(0, line_number);

	uint8_t x = redisplay_line(_cursor.line);
	line_t *it = _cursor.line->next;
	while (it != 0 && line_number < (_height - 1))
	{
		redisplay_line(it);
		sys_chan_write_b(0, '\n');

		line_number++;
		it = it->next;
	}

	_cursor_x = x;
	_cursor_y = line_number;
	con_set_xy(_cursor_x, _cursor_y);
}



/** Commands **/

static bool cmd_quit(uint8_t ch)
{
	con_teardown();
	sys_exit(0);
	return true;
}

static bool cmd_insert_char(uint8_t ch)
{
	line_t *line = _cursor.line;

	// Make room for another char
	if (line->len == line->cap)
	{
		line_t *old = line;
		line = grow_line(line);

		if (line != old)
		{
			_cursor.line = line;
			if (_scroll.line == old)
				_scroll.line = line;
		}
	}

	uint8_t len = line->len - _cursor.offset;
	for (uint8_t i = 1; i < len; ++i)
		line->data[line->len - i] = line->data[line->len - i - 1];

	line->data[_cursor.offset] = ch;
	line->len++;
	_cursor.offset++;

	redisplay_current_line();

	//redisplay_current_line();
	//update_cursor();

	return true;
}

static bool cmd_insert_newline(uint8_t ch)
{
	uint8_t line_count = get_current_line_number();

	line_t *current_line = _cursor.line;
	line_t *new_line = alloc_line(8);

	new_line->next = current_line->next;
	new_line->prev = current_line;

	if (current_line->next != 0)
		current_line->next->prev = new_line;

	current_line->next = new_line;

	// Copy data from the current line to the new one
	if (_cursor.offset < current_line->len)
	{
		uint8_t left = current_line->len - _cursor.offset;
		memcpy(new_line->data, current_line->data + _cursor.offset, left);
		//memset(current_line->data + _cursor.offset, ' ', left);

		current_line->len = _cursor.offset;
		new_line->len = left;
	}

	_cursor.line = new_line;
	_cursor.offset = 0;

	if (line_count >= _height - 2)
	{
		_scroll.line = _scroll.line->next;
		redisplay_all();
	}
	else
	{
		redisplay_current_line_down();
	}

	return true;
}

static bool cmd_backspace(uint8_t ch)
{
	if (_cursor.offset > 0)
	{
		line_t * line = _cursor.line;
		uint8_t i;

		--_cursor.offset;

		for (i = _cursor.offset; i < line->len; ++i)
			line->data[i] = line->data[i+1];

		line->len--;
	}
	else
	{
		// Here we need to merge the current line with the previous one.
	}


	redisplay_current_line();
	return true;
}


/** Main **/

int main(int argc, char * argv[])
{
	char buffer[64];
	char * msg = "Hello, World\n";

	con_setup();
	con_get_size(&_width, &_height);


	memset(_basic_commands, 0, sizeof(_basic_commands));
	_basic_commands[CON_KEY_CTRL_Q] = cmd_quit;
	_basic_commands['a'] = cmd_insert_char;
	_basic_commands['b'] = cmd_insert_char;
	_basic_commands['c'] = cmd_insert_char;
	_basic_commands['d'] = cmd_insert_char;
	_basic_commands['e'] = cmd_insert_char;
	_basic_commands['f'] = cmd_insert_char;
	_basic_commands['g'] = cmd_insert_char;
	_basic_commands['h'] = cmd_insert_char;
	_basic_commands['i'] = cmd_insert_char;
	_basic_commands['j'] = cmd_insert_char;
	_basic_commands['k'] = cmd_insert_char;
	_basic_commands['l'] = cmd_insert_char;
	_basic_commands['m'] = cmd_insert_char;
	_basic_commands['n'] = cmd_insert_char;
	_basic_commands['o'] = cmd_insert_char;
	_basic_commands['p'] = cmd_insert_char;
	_basic_commands['q'] = cmd_insert_char;
	_basic_commands['r'] = cmd_insert_char;
	_basic_commands['s'] = cmd_insert_char;
	_basic_commands['t'] = cmd_insert_char;
	_basic_commands['u'] = cmd_insert_char;
	_basic_commands['v'] = cmd_insert_char;
	_basic_commands['w'] = cmd_insert_char;
	_basic_commands['x'] = cmd_insert_char;
	_basic_commands['y'] = cmd_insert_char;
	_basic_commands['z'] = cmd_insert_char;
	_basic_commands['0'] = cmd_insert_char;
	_basic_commands['1'] = cmd_insert_char;
	_basic_commands['2'] = cmd_insert_char;
	_basic_commands['3'] = cmd_insert_char;
	_basic_commands['4'] = cmd_insert_char;
	_basic_commands['5'] = cmd_insert_char;
	_basic_commands['6'] = cmd_insert_char;
	_basic_commands['7'] = cmd_insert_char;
	_basic_commands['8'] = cmd_insert_char;
	_basic_commands['9'] = cmd_insert_char;
	_basic_commands[','] = cmd_insert_char;
	_basic_commands['.'] = cmd_insert_char;
	_basic_commands['-'] = cmd_insert_char;
	_basic_commands['_'] = cmd_insert_char;
	_basic_commands['?'] = cmd_insert_char;
	_basic_commands['!'] = cmd_insert_char;
	_basic_commands['"'] = cmd_insert_char;
	_basic_commands['#'] = cmd_insert_char;
	_basic_commands['$'] = cmd_insert_char;
	_basic_commands['%'] = cmd_insert_char;
	_basic_commands['&'] = cmd_insert_char;
	_basic_commands['/'] = cmd_insert_char;
	_basic_commands['\\'] = cmd_insert_char;
	_basic_commands['{'] = cmd_insert_char;
	_basic_commands['}'] = cmd_insert_char;
	_basic_commands['['] = cmd_insert_char;
	_basic_commands[']'] = cmd_insert_char;
	_basic_commands['('] = cmd_insert_char;
	_basic_commands[')'] = cmd_insert_char;
	_basic_commands['='] = cmd_insert_char;
	_basic_commands['+'] = cmd_insert_char;
	_basic_commands['?'] = cmd_insert_char;
	_basic_commands['*'] = cmd_insert_char;
	_basic_commands['<'] = cmd_insert_char;
	_basic_commands['>'] = cmd_insert_char;
	_basic_commands['|'] = cmd_insert_char;
	_basic_commands[':'] = cmd_insert_char;
	_basic_commands[';'] = cmd_insert_char;
	_basic_commands[' '] = cmd_insert_char;
	_basic_commands['\t'] = cmd_insert_char;
	_basic_commands[0x0D] = cmd_insert_newline;
	_basic_commands[0x08] = cmd_backspace;

	_current_commands = _basic_commands;


	new_document();

	int initial_size = sizeof(line_t) + 8 - 1;
	int size = ((initial_size / 4) + 1) * 4;

	int len = snprintf(buffer, 64, "%dx%d  %d -> %d\n", _width, _height, initial_size, size);
	sys_chan_write(0, buffer, len);


	display_statusbar("Foenix Text Editor, Ctrl+Q to quit");
	con_set_xy(0, 2);

	while (true)
	{
		uint8_t key = con_get_key();

		command_t cmd = _current_commands[key];
		if (cmd != 0)
			cmd(key);

		snprintf(buffer, 64, "%c (%04X)  (%d, %d) %d Kb free", (key > 32 && key <= 126) ? (char)key : '.', key, _cursor.line->len, _cursor.line->cap, mem_free() / 1024);
		display_statusbar(buffer);
	}

	return 0;
}