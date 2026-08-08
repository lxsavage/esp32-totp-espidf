#include "storage.h"

static bool initialized = false;
void storage_init()
{
    if (initialized)
        return;

    // nvs_flash_init();
}

void storage_load_privatekey(struct storage_OTPCode* out) {}

void storage_load_wifi(struct storage_WiFiDetails* out) {}

void storage_write_secret(struct storage_OTPCode* in) {}

void storage_write_wifi(struct storage_WiFiDetails* in) {}

_Bool storage_commit_writes() { return false; }
