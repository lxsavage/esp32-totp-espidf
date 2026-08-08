#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "constants.h"

struct storage_OTPCode
{
    size_t label_len;
    size_t key_len;
    char label[256];
    unsigned char key[TOTP_KEY_MAX];
};

struct storage_WiFiDetails
{
    char ppk[64];
    char ssid[32];
};

// Initialize storage for reading/writing
void storage_init();

// Get the TOTP secret from storage
void storage_load_privatekey(struct storage_OTPCode* out);

// Get the WiFi credentials from storage
void storage_load_wifi(struct storage_WiFiDetails* out);

// Write a new secret to storage
void storage_write_secret(struct storage_OTPCode* in);

// Write new WiFi credentials to storage
void storage_write_wifi(struct storage_WiFiDetails* in);

// Apply any pending write calls to storage
_Bool storage_commit_writes();

#endif
