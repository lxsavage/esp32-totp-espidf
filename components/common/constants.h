#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED

#include <stdint.h>

// Period for re-syncing time with NTP server (seconds); uses deep sleep between
// resynchronization events, so this is not an exact timing
#define TIME_SYNC_INTERVAL_SEC 86400 // 1 day

// Minimum amount of time to display label before code (if applicable)
#define LABEL_READ_TIME_MS 1500

// Serial communication
#define UART_BAUD_RATE 115200
#define UART_PORT UART_NUM_0
#define UART_BUF_SIZE 256

// Other constants
#define TOTP_KEY_MAX 128
#define TOTP_POLL_uS 1000000

#define TOTP_DISP_EXP_START_COL 11
#define TOTP_DISP_UNDERLAY "Expires in   s  "
// Number mask for ref  -> "           XX   "

#endif
