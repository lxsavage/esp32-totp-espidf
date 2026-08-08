#ifndef SERIAL_H_INCLUDED
#define SERIAL_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

// Open a serial connection to the device. Returns a file descriptor for the
// opened connection.
int open_port(const char* device);

// Sends a message over the serial port defined in the fd file descriptor.
// Returns true if successful, false otherwise.
_Bool send_serial(int fd, const char* message, size_t message_len);

#endif
