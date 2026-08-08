#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED

#include <stdint.h>

// Uncomment to allow for debug aborts to happen as well as additional logging
#define DEBUG

// Uncomment to directly bypass load mode and load credentials into storage;
// requires DEBUG to be set!
// #define LOAD_TEST

// Pinout for LCD to display codes/status messages
#define RS 13
#define ENABLE 14
#define D4 26
#define D5 25
#define D6 18
#define D7 19

// Pinout for pushbutton to enter load mode
#define LOAD_BTN 21

// Period for re-syncing time with NTP server (seconds); uses deep sleep between
// resynchronization events, so this is not an exact timing
#define TIME_SYNC_INTERVAL_SEC 86400 // 1 day

// Minimum amount of time to display label before code (if applicable)
#define LABEL_READ_TIME 1500 // 1.5 seconds

// Other constants
#define BAUD_RATE 115200
#define TOTP_KEY_MAX 128
#define TOTP_POLL_NS 1000000 // 1 second

// Debug helper
#include <debug.h>

#endif
