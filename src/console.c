
#include "console.h"
#include "syscalls.h"
#include "vicky3.h"


static int16_t _columns_visible;
static int16_t _columns_max;
static int16_t _rows_visible;
static int16_t _rows_max;
static int16_t _cursor_x;
static int16_t _cursor_y;
static uint8_t _current_color;
static volatile char * _text_cursor_ptr;
static volatile char * _color_cursor_ptr;



static void con_setup_size(void)
{
    uint32_t border = *BORDER_CONTROL_REG_A;
    int16_t pixel_double = *MASTER_CONTROL_REG_A & VKY3_MCR_DOUBLE_EN;
    int16_t resolution = (*MASTER_CONTROL_REG_A & VKY3_MCR_RESOLUTION_MASK) >> 8;

    /* Set number of maximum rows and columns based on the base resolution */
    switch (resolution)
    {
        case 0: /* 640x480 */
            _columns_max = 80;
            _rows_max = 60;
            break;

        case 1: /* 800x600 */
            _columns_max = 100;
            _rows_max = 75;
            break;

        case 2: /* 1024x768 */
            _columns_max = 128;
            _rows_max = 96;
            break;

        case 3: /* 640x400 */
            _columns_max = 80;
            _rows_max = 50;
            break;

        default:
            break;
    }

    // If we are pixel doubling, characters are twice as big
    if (pixel_double)
    {
        _columns_max /= 2;
        _rows_max /= 2;
    }

    _columns_visible = _columns_max;
    _rows_visible = _rows_max;

    /* If the border is enabled, subtract it from the visible rows and columns */
    if (border & VKY3_BRDR_EN)
    {
        int16_t border_width = (border & VKY3_X_SIZE_MASK) >> 8;
        int16_t border_height = (border & VKY3_Y_SIZE_MASK) >> 16;

        int16_t columns_reduction = border_width / 4;
        int16_t rows_reduction = border_height / 4;

        if (pixel_double)
        {
            columns_reduction /= 2;
            rows_reduction /= 2;
        }

        _columns_visible -= columns_reduction;
        _rows_visible -= rows_reduction;
    }
}


void con_setup(void)
{
    *MASTER_CONTROL_REG_A = VKY3_MCR_640x480 | VKY3_MCR_TEXT_EN;      /* Set to text only mode: 640x480 */

    con_set_border(true, 0x10, 0x10, 0x00000000);
    con_set_color(CON_COLOR_GREY, CON_COLOR_BLUE);
    con_set_cursor(0xF3, 0x7F, 1, true);

    con_setup_size();

    con_clear_screen();
    con_set_xy(0, 0);

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

void con_set_border(bool visible, int16_t width, int16_t height, uint32_t color)
{
    if (visible)
    {
        /* Set the width and color */
        BORDER_CONTROL_REG_A[0] = ((height & 0xff) << 16) | ((width & 0xff) << 8) | 1;
        BORDER_CONTROL_REG_A[1] = (color & 0x00ff0000) | ((color & 0xff) << 8) | ((color & 0xff00) >> 8);
    }
    else
    {
        /* Hide the border and make it 0 width */
        BORDER_CONTROL_REG_A[0] = 0;
        BORDER_CONTROL_REG_A[1] = 0;
    }
}

void con_set_cursor(int16_t color, uint8_t character, int16_t rate, bool enable)
{
    *CURSOR_SETTINGS_REG_A = ((color & 0xff) << 24) | (character << 16) | ((rate & 0x02) << 1) | (enable ? 0x01 : 0x00);
}

void con_clear_screen(void)
{
    for (uint32_t i = 0; i < 0x2000; ++i)
    {
        SCREEN_TEXT_A[i] = ' ';
        COLOR_TEXT_A[i] = _current_color;
    }
}

void con_clear_line(void)
{
    int32_t sol_index = _cursor_y * _columns_max;
    int32_t eol_index = (_cursor_y + 1) * _columns_max;

    for (int32_t i = sol_index; i < eol_index; ++i)
    {
        SCREEN_TEXT_A[i] = ' ';
        COLOR_TEXT_A[i] = _current_color;
    }
}

void con_set_xy(uint16_t x, uint16_t y)
{
    if (x >= _columns_visible)
    {
        x = 0;
        y++;
    }

    if (y >= _rows_visible)
        y = _rows_visible - 1;

    _cursor_x = x;
    _cursor_y = y;

    *(CURSOR_POSITION_REG_A) = ((uint32_t)y << 16) | (uint32_t)x;
    int16_t offset = y * _columns_max + x;
    _text_cursor_ptr = &SCREEN_TEXT_A[offset];
    _color_cursor_ptr = &COLOR_TEXT_A[offset];
}

void con_set_color(int16_t foreground, int16_t background)
{
    _current_color = ((foreground & 0x0f) << 4) | (background & 0x0f);  
}

void con_get_size(int16_t *x, int16_t *y)
{
    *x = _columns_visible;
    *y = _rows_visible;
}

void con_out(uint8_t ch)
{
    switch (ch)
    {
        case CON_CHAR_NL:
            con_set_xy(0, _cursor_y + 1);
            break;

        case CON_CHAR_CR:
            break;

        case CON_CHAR_BS:
            con_set_xy(_cursor_x - 1, _cursor_y);
            *_text_cursor_ptr = ' ';
            *_color_cursor_ptr = _current_color;
            break;

        default:
            *_text_cursor_ptr++ = ch;
            *_color_cursor_ptr++ = _current_color;
            con_set_xy(_cursor_x + 1, _cursor_y);
            break;
    }
}

void con_out_raw(uint8_t ch)
{
    *_text_cursor_ptr++ = ch;
    *_color_cursor_ptr++ = _current_color;
}

void con_newline(void)
{
    con_set_xy(0, _cursor_y + 1);
}

void con_write(uint8_t * buffer, uint16_t size)
{
    for (uint16_t i = 0; i < size; ++i)
        con_out(buffer[i]);
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
                    case 'A':   // Up
                        return CON_KEY_UP;

                    case 'B':   // Left
                        return CON_KEY_DOWN;

                    case 'C':   // Right
                        return CON_KEY_RIGHT;

                    case 'D':   // Down
                        return CON_KEY_LEFT;

                    case '1':
                        {
                            key = sys_chan_read_b(0);
                            switch (key)
                            {
                                case '~':
                                    return CON_KEY_HOME;
                                case '1':
                                    key = sys_chan_read_b(0);   // read last ~
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
                                    key = sys_chan_read_b(0);   // read last ~
                                    return CON_KEY_F9;
                                case '1':
                                    key = sys_chan_read_b(0);   // read last ~
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