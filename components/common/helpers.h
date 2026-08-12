#ifndef HELPERS_H_INCLUDED
#define HELPERS_H_INCLUDED

#include "constants.h"

#ifdef SERIAL_NOTES_ENABLED
void h_serial_msg(const char* command, const char* data, const char* rem);
// Writes a properly-formatted serial message for use on UART communication
#define SERIAL_MSG(x, y, z) h_serial_msg(x, y, z)
#else
// Writes a properly-formatted serial message for use on UART communication
#define SERIAL_MSG(x, y, z)
#endif

#ifdef DEBUG
// If DEBUG is enabled, surround this call with an error assert
#define ERR_CHECK(x) ESP_ERROR_CHECK(x)
#else
// If DEBUG is enabled, surround this call with an error assert
#define ERR_CHECK(x) (void)(x)
#endif

#endif
