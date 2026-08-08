#include "serial.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

// Hack to account for CRTSCTS for some reason not showing up as defined in
// vscode despite being in termios.h, and not wanting to adjust it in the C/Cpp
// settings as to not interfere with the main embedded program.
//
// This define shouldn't happen on any POSIX-compatible compiler. This utility
// doesn't target non-POSIX-compatible systems, so this should be fine.
#ifndef CRTSCTS
#define CRTSCTS 020000000000
#endif

int open_port(const char* device)
{
    int fd;
    if ((fd = open(device, O_RDWR | O_NOCTTY | O_SYNC)) < 0)
        return -1;

    struct termios tty;
    tcgetattr(fd, &tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag &= ~PARENB; // No parity
    tty.c_cflag &= ~CSTOPB; // 1 stop bit
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            // 8 data bits
    tty.c_cflag &= ~CRTSCTS;       // No hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable receiver, local mode
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // No software flow control
    tty.c_oflag &= ~OPOST;                          // Raw output

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    tcsetattr(fd, TCSANOW, &tty);
    return fd;
}

_Bool send_serial(int fd, const char* message, size_t message_len)
{
    write(fd, message, message_len);
    write(fd, "\n", 1);
    if (tcflush(fd, TCIFLUSH) < 0)
        return false;

    return true;
}
