#ifndef HELPERS_H_INCLUDED
#define HELPERS_H_INCLUDED

#include "constants.h"

#if defined(SERIAL_NOTES_ENABLED) || defined(LOAD_MODE_ENABLED)
void h_serial_msg(const char* command, const char* data, const char* rem);
#define SERIAL_MSG(x, y, z) h_serial_msg(x, y, z)
#define SERIAL_NOTES_AVAILABLE
#else
#define SERIAL_MSG(x, y, z)
#endif

#ifdef DEBUG
#define ERR_CHECK(x) ESP_ERROR_CHECK(x)
#else
#define ERR_CHECK(x) x
#endif

#endif
