#ifndef LOAD_MODE_H_INCLUDED
#define LOAD_MODE_H_INCLUDED

#include "constants.h"

#ifdef LOAD_MODE_ENABLED
// Enter load mode to receive a new secret as well as potentially new WiFi
// credentials, then exit.
void load_mode();
#endif

#endif
