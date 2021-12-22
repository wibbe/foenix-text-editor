
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

static char _buffer_prompt[32] = {0};
static uint16_t _buffer_prompt_len;
static line_t *_buffer_line = 0;
static bool _in_buffer = false;
static command_t _buffer_accept_cmd;
static command_t _buffer_reject_cmd;
static location_t _buffer_old_cursor;


static command_t _basic_commands[256];
static command_t _buffer_commands[256];
static command_t * _current_commands;

static line_t _line_cache_8 = {0};
static line_t _line_cache_32 = {0};
static line_t _line_cache_64 = {0};
static line_t _line_cache_128 = {0};




static void display_statusbar(char * msg);



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

static uint16_t get_line_number(line_t *current)
{
	if (current == _buffer_line)
		return _height - 1;

	line_t * line = _scroll.line;
	uint16_t line_number = 0;

	while (line != 0 && line != current)
	{
		line = line->next;
		++line_number;
	}

	return line_number;
}

static uint16_t get_current_line_number(void)
{
	return get_line_number(_cursor.line);
}

static void update_cursor(void)
{
	line_t *line = _cursor.line;

	_cursor_x = _in_buffer ? _buffer_prompt_len : 0;

	for (uint8_t i = 0; i < _cursor.offset; ++i)
	{
		uint8_t ch = line->data[i];

		if (ch == '\t')
		{
			uint8_t tab_count = _cursor_x / 2;
			_cursor_x = (tab_count + 1) * 2;
		}
		else
		{
			++_cursor_x;
		}
	}

	_cursor_y = get_current_line_number();
	con_set_xy(_cursor_x, _cursor_y);
}


static void display_line(line_t *line)
{
	uint8_t pos = 0;
	for (uint8_t i = 0; i < _width; ++i)
	{
		if (i < line->len)
		{
			uint8_t ch = line->data[i];

			if (ch == '\t')
			{
				uint8_t tab_count = pos / 2;
				uint8_t new_pos = (tab_count + 1) * 2;

				while (pos < new_pos)
				{
					++pos;
					con_out_raw(' ');
				}
			}
			else
			{
				++pos;
				con_out_raw(ch);
			}
		}
		else
		{
			con_out_raw(' ');
		}
	}
}

static void redisplay_current_line(void)
{
	if (_in_buffer)
	{
		display_statusbar(0);
	}
	else
	{
		uint16_t line_number = get_line_number(_cursor.line);
		con_set_xy(0, line_number);
		display_line(_cursor.line);
	}

	update_cursor();
}

static void redisplay_all(void)
{
	con_set_xy(0, 0);

	uint16_t line_number = 0;
	uint8_t x = _cursor.offset;
	line_t *it = _scroll.line;

	while (it != 0 && line_number < (_height - 1))
	{
		display_line(it);
		con_newline();
		
		line_number++;
		it = it->next;
	}

	update_cursor();
}

static void redisplay_line_down(line_t *line)
{
	uint16_t line_number = get_line_number(line);
	con_set_xy(0, line_number);

	while (line != 0 && line_number < (_height - 1))
	{
		display_line(line);
		con_newline();

		line_number++;
		line = line->next;
	}

	update_cursor();
}

static void display_statusbar(char * msg)
{
	con_set_xy(0, _height - 1);
	con_set_color(CON_COLOR_BLUE, CON_COLOR_GREY);

	if (_in_buffer)
	{
		con_write(_buffer_prompt, _buffer_prompt_len);
		display_line(_buffer_line);
	}
	else
	{
		int len = strlen(msg);
		con_clear_line();
		con_out(' ');
		con_write(msg, len);
	}
	con_set_color(CON_COLOR_GREY, CON_COLOR_BLUE);
	con_set_xy(_cursor_x, _cursor_y);
}

/** Buffer Handling **/

static bool buffer_generic_reject(uint8_t ch)
{
	_in_buffer = false;
	_cursor = _buffer_old_cursor;
	_current_commands = _basic_commands;

	update_cursor();
}

static void enter_buffer(char *prompt, command_t accept, command_t reject)
{
	_in_buffer = true;
	_buffer_accept_cmd = accept;
	_buffer_reject_cmd = reject == 0 ? buffer_generic_reject : reject;
	_buffer_line->len = 0;

	_buffer_old_cursor = _cursor;
	_cursor.line = _buffer_line;
	_cursor.offset = 0;

	_current_commands = _buffer_commands;

	_buffer_prompt_len = strlen(prompt);
	if (_buffer_prompt_len > sizeof(_buffer_prompt))
		_buffer_prompt_len = sizeof(_buffer_prompt_len);

	memcpy(_buffer_prompt, prompt, _buffer_prompt_len);

	display_statusbar(0);
	update_cursor();
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

	uint8_t len = line->cap - _cursor.offset;
	for (uint8_t i = 1; i < len; ++i)
		line->data[line->cap - i] = line->data[line->cap - i - 1];

	line->data[_cursor.offset] = ch;
	line->len++;
	_cursor.offset++;

	redisplay_current_line();
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
		redisplay_line_down(current_line);
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

static bool cmd_move_up(uint8_t ch)
{
	if (_cursor.line->prev != 0)
	{
		if (_scroll.line == _cursor.line)
			_scroll.line = _scroll.line->prev;

		_cursor.line = _cursor.line->prev;
		if (_cursor.line->len < _cursor.offset)
			_cursor.offset = _cursor.line->len;
	}

	update_cursor();
	return true;
}

static bool cmd_move_down(uint8_t ch)
{
	uint8_t line_count = get_current_line_number();

	if (_cursor.line->next != 0)
	{
		if (line_count >= _height - 1)
			_scroll.line = _scroll.line->next;

		_cursor.line = _cursor.line->next;
		if (_cursor.line->len < _cursor.offset)
			_cursor.offset = _cursor.line->len;
	}

	update_cursor();
	return true;
}

static bool cmd_move_left(uint8_t ch)
{
	if (_cursor.offset > 0)
	{
		--_cursor.offset;
		update_cursor();
	}
	else if (!_in_buffer)
	{
		_cursor.offset = 255;	// this ensures we are placed at the end of the line
		cmd_move_up(ch);		
	}

	return true;
}

static bool cmd_move_right(uint8_t ch)
{
	if (_cursor.offset < _cursor.line->len)
	{
		++_cursor.offset;
		update_cursor();
	}
	else if (!_in_buffer)
	{
		_cursor.offset = 0;
		cmd_move_down(ch);
	}
	return true;
}

static bool cmd_document_save_as(uint8_t ch)
{
	enter_buffer("Save as:", 0, 0);
	return true;
}

static bool cmd_reject_buffer(uint8_t ch)
{
	if (_in_buffer && _buffer_reject_cmd)
		_buffer_reject_cmd(ch);

	return true;
}


/** Main **/

static void add_char_commands(command_t *commands)
{
	commands['a'] = cmd_insert_char;
	commands['b'] = cmd_insert_char;
	commands['c'] = cmd_insert_char;
	commands['d'] = cmd_insert_char;
	commands['e'] = cmd_insert_char;
	commands['f'] = cmd_insert_char;
	commands['g'] = cmd_insert_char;
	commands['h'] = cmd_insert_char;
	commands['i'] = cmd_insert_char;
	commands['j'] = cmd_insert_char;
	commands['k'] = cmd_insert_char;
	commands['l'] = cmd_insert_char;
	commands['m'] = cmd_insert_char;
	commands['n'] = cmd_insert_char;
	commands['o'] = cmd_insert_char;
	commands['p'] = cmd_insert_char;
	commands['q'] = cmd_insert_char;
	commands['r'] = cmd_insert_char;
	commands['s'] = cmd_insert_char;
	commands['t'] = cmd_insert_char;
	commands['u'] = cmd_insert_char;
	commands['v'] = cmd_insert_char;
	commands['w'] = cmd_insert_char;
	commands['x'] = cmd_insert_char;
	commands['y'] = cmd_insert_char;
	commands['z'] = cmd_insert_char;
	commands['A'] = cmd_insert_char;
	commands['B'] = cmd_insert_char;
	commands['C'] = cmd_insert_char;
	commands['D'] = cmd_insert_char;
	commands['E'] = cmd_insert_char;
	commands['F'] = cmd_insert_char;
	commands['G'] = cmd_insert_char;
	commands['H'] = cmd_insert_char;
	commands['I'] = cmd_insert_char;
	commands['J'] = cmd_insert_char;
	commands['K'] = cmd_insert_char;
	commands['L'] = cmd_insert_char;
	commands['M'] = cmd_insert_char;
	commands['N'] = cmd_insert_char;
	commands['O'] = cmd_insert_char;
	commands['P'] = cmd_insert_char;
	commands['Q'] = cmd_insert_char;
	commands['R'] = cmd_insert_char;
	commands['S'] = cmd_insert_char;
	commands['T'] = cmd_insert_char;
	commands['U'] = cmd_insert_char;
	commands['V'] = cmd_insert_char;
	commands['W'] = cmd_insert_char;
	commands['X'] = cmd_insert_char;
	commands['Y'] = cmd_insert_char;
	commands['Z'] = cmd_insert_char;	
	commands['0'] = cmd_insert_char;
	commands['1'] = cmd_insert_char;
	commands['2'] = cmd_insert_char;
	commands['3'] = cmd_insert_char;
	commands['4'] = cmd_insert_char;
	commands['5'] = cmd_insert_char;
	commands['6'] = cmd_insert_char;
	commands['7'] = cmd_insert_char;
	commands['8'] = cmd_insert_char;
	commands['9'] = cmd_insert_char;
	commands[','] = cmd_insert_char;
	commands['.'] = cmd_insert_char;
	commands['-'] = cmd_insert_char;
	commands['_'] = cmd_insert_char;
	commands['?'] = cmd_insert_char;
	commands['!'] = cmd_insert_char;
	commands['"'] = cmd_insert_char;
	commands['#'] = cmd_insert_char;
	commands['$'] = cmd_insert_char;
	commands['%'] = cmd_insert_char;
	commands['&'] = cmd_insert_char;
	commands['/'] = cmd_insert_char;
	commands['\\'] = cmd_insert_char;
	commands['{'] = cmd_insert_char;
	commands['}'] = cmd_insert_char;
	commands['['] = cmd_insert_char;
	commands[']'] = cmd_insert_char;
	commands['('] = cmd_insert_char;
	commands[')'] = cmd_insert_char;
	commands['='] = cmd_insert_char;
	commands['+'] = cmd_insert_char;
	commands['?'] = cmd_insert_char;
	commands['*'] = cmd_insert_char;
	commands['<'] = cmd_insert_char;
	commands['>'] = cmd_insert_char;
	commands['|'] = cmd_insert_char;
	commands[':'] = cmd_insert_char;
	commands[';'] = cmd_insert_char;
	commands[' '] = cmd_insert_char;
}


int main(int argc, char * argv[])
{
	char buffer[64];
	char * msg = "Hello, World\n";

	con_setup();
	con_get_size(&_width, &_height);


	memset(_basic_commands, 0, sizeof(_basic_commands));
	memset(_buffer_commands, 0, sizeof(_buffer_commands));

	add_char_commands(_basic_commands);
	add_char_commands(_buffer_commands);

	_basic_commands[CON_KEY_CTRL_Q] = cmd_quit;
	_basic_commands[CON_KEY_CTRL_S] = cmd_document_save_as;
	_basic_commands['\t'] = cmd_insert_char;
	_basic_commands[CON_KEY_ENTER] = cmd_insert_newline;
	_basic_commands[CON_KEY_BACKSPACE] = cmd_backspace;
	_basic_commands[CON_KEY_LEFT] = cmd_move_left;
	_basic_commands[CON_KEY_RIGHT] = cmd_move_right;
	_basic_commands[CON_KEY_UP] = cmd_move_up;
	_basic_commands[CON_KEY_DOWN] = cmd_move_down;

	_buffer_commands[CON_KEY_ESC] = cmd_reject_buffer;
	_buffer_commands[CON_KEY_BACKSPACE] = cmd_backspace;
	_buffer_commands[CON_KEY_LEFT] = cmd_move_left;
	_buffer_commands[CON_KEY_RIGHT] = cmd_move_right;

	_current_commands = _basic_commands;


	new_document();
	update_cursor();

	display_statusbar("Foenix Text Editor, Ctrl+Q to quit");
		
	_in_buffer = false;
	_buffer_line = alloc_line(128);

	while (true)
	{
		uint8_t key = con_get_key();

		command_t cmd = _current_commands[key];
		if (cmd != 0)
			cmd(key);

		if (!_in_buffer)
		{
			snprintf(buffer, 64, "%c (%04X) %d (%d, %d) %d Kb free", (key > 32 && key <= 126) ? (char)key : '.', key, key == CON_KEY_LEFT, _cursor.line->len, _cursor.line->cap, mem_free() / 1024);
			display_statusbar(buffer);
		}
	}

	return 0;
}