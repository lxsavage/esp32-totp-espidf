#ifndef HELPERS_H_INCLUDED
#define HELPERS_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

// Decode base32 from encoded into decoded up to maxbuf chars
int base32decode(const char* encoded, unsigned char* decoded, size_t maxbuf);

// Returns true if test points to a string with a valid base32-encoded string,
// otherwise returns false
_Bool base32valid(const char* test);

#endif
