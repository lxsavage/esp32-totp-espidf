#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

// Uncomment to enable load mode triggering (WIP)
#define LOAD_MODE_ENABLED

// Uncomment to enable command notes being sent over UART
#define SERIAL_NOTES_ENABLED

// Uncomment to disable restart after load mode
// #define STALL_ON_LOAD_COMPLETE

// Uncomment to allow for debug aborts to happen as well as additional logging
// #define DEBUG

// Pinout for LCD to display codes/status messages
#define RS 13
#define ENABLE 14
#define D4 26
#define D5 25
#define D6 18
#define D7 19

// Pinout for pushbutton to enter load mode
#define LOAD_BTN 21

#endif
