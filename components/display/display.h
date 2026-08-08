#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

#include <stdbool.h>

_Bool display_init(int rs, int enable, int d4, int d5, int d6, int d7);
_Bool display_begin(int cols, int rows);

void display_set_cursor(int col, int row);

void display_clear();
void display_write(const char* content);
void display_write_byte(char byteval);

#endif
