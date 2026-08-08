#ifndef TIME_H_INCLUDED
#define TIME_H_INCLUDED

#include <stdbool.h>

#include "storage.h"

// Sync the clock to the current time; this is NOT thread safe
_Bool rtc_sync(struct storage_WiFiDetails* wifi, _Bool print_errors);

// Get whether the time has been configured yet; thread safe
_Bool rtc_ready();

// Get the current time; thread safe
unsigned long rtc_get();

#endif
