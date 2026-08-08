#include "display.h"

#include "constants.h"

#include "hd44780.h"

static lcd_bus_hd44780_t* bus = NULL;
static hd44780_t* lcd = NULL;

_Bool display_init(int rs, int enable, int d4, int d5, int d6, int d7)
{
    bus = lcd_bus_gpio4_create(RS, ENABLE, D4, D5, D6, D7);
    return bus != NULL;
}

_Bool display_begin(int cols, int rows)
{
    if (cols == 16 && rows == 2)
        lcd = lcd_init(bus, HD44780_GEOMETRY_16X2, true);
    else if (cols == 16 && rows == 4)
        lcd = lcd_init(bus, HD44780_GEOMETRY_16X4, true);
    else if (cols == 20 && rows == 2)
        lcd = lcd_init(bus, HD44780_GEOMETRY_20X2, true);
    else if (cols == 20 && rows == 4)
        lcd = lcd_init(bus, HD44780_GEOMETRY_20X4, true);
    else if (cols == 20 && rows == 2)
        lcd = lcd_init(bus, HD44780_GEOMETRY_40X2, true);
    else
        return false;

    return true;
}

void display_set_cursor(int col, int row) { lcd_set_cursor(lcd, col, row); }

void display_clear() { lcd_clear_screen(lcd); }

void display_write(const char* content) { lcd_write_str(lcd, content); }

void display_write_byte(char byteval) { lcd_write_char(lcd, byteval); }
