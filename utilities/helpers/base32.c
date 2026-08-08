#include "base32.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int base32decode(const char* encoded, unsigned char* decoded, size_t maxbuf)
{
    static const int8_t base32_vals[256] = {
        ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,  ['F'] = 5,
        ['G'] = 6,  ['H'] = 7,  ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11,
        ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15, ['Q'] = 16, ['R'] = 17,
        ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
        ['Y'] = 24, ['Z'] = 25, ['2'] = 26, ['3'] = 27, ['4'] = 28, ['5'] = 29,
        ['6'] = 30, ['7'] = 31, ['a'] = 0,  ['b'] = 1,  ['c'] = 2,  ['d'] = 3,
        ['e'] = 4,  ['f'] = 5,  ['g'] = 6,  ['h'] = 7,  ['i'] = 8,  ['j'] = 9,
        ['k'] = 10, ['l'] = 11, ['m'] = 12, ['n'] = 13, ['o'] = 14, ['p'] = 15,
        ['q'] = 16, ['r'] = 17, ['s'] = 18, ['t'] = 19, ['u'] = 20, ['v'] = 21,
        ['w'] = 22, ['x'] = 23, ['y'] = 24, ['z'] = 25,
    };

    size_t out_len = 0;
    uint32_t buffer = 0;
    int bits_left = 0;

    for (; *encoded; ++encoded)
    {
        int8_t val = base32_vals[(unsigned char)*encoded];
        if (val < 0)
        {
            if (*encoded == '=')
                break;
            continue; // ignore invalid chars
        }

        buffer = (buffer << 5) | val;
        bits_left += 5;

        if (bits_left >= 8)
        {
            bits_left -= 8;
            if (out_len >= maxbuf)
                return -1; // buffer overflow
            decoded[out_len++] = (buffer >> bits_left) & 0xFF;
        }
    }

    decoded[out_len] = '\0';
    return (int)out_len;
}

_Bool base32valid(const char* test)
{
    size_t len = strlen(test);
    if (len == 0)
        return false;

    for (size_t i = 0; i < len; i++)
    {
        char c = test[i];
        if (('A' <= c && c <= 'Z') || ('2' <= c && c <= '7'))
            continue;

        return false;
    }

    return true;
}
