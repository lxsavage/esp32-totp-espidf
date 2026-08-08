#include "display.h"

_Bool display_init(int rs, int enable, int d4, int d5, int d6, int d7)
{
    return false;
}

_Bool display_begin(int cols, int rows) { return false; }

void display_set_cursor(int col, int row) {}

void display_clear() {}

void display_write(const char* content) {}

void display_write_byte(char byteval) {}
