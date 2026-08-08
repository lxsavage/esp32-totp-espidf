#ifndef TOTP_H_INCLUDED
#define TOTP_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#include "storage.h"

// Initialize crypto libraries needed for generating TOTP codes
void totp_init();

// Calculate a TOTP code for the given key and time
// - key/key_len: the decoded private key and its character count.
// - time: the current unix timestamp, which is canonically a
//         32-bit integer
// - out: a buffer of at least 7 bytes to store the output/terminator in
//
// Returns true if successful, false otherwise.
_Bool totp_generate(const uint8_t* key, size_t key_len, uint64_t time,
                    char out[7]);

#endif
