#include "totp.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <mbedtls/md.h>

#include "storage.h"

static int hmac_sha1_md(const uint8_t* key, size_t key_len, const uint8_t* msg,
                        size_t msg_len, uint8_t out[20])
{
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (!info)
        return -1;

    const size_t block = 64;                       // SHA-1 block size
    const size_t dlen = mbedtls_md_get_size(info); // 20
    uint8_t k0[64] = {0};
    uint8_t ipad[64];
    uint8_t opad[64];
    uint8_t tmp[20];

    if (key_len > block)
    {
        // K0 = H(key)
        mbedtls_md_context_t kctx;
        mbedtls_md_init(&kctx);
        if (mbedtls_md_setup(&kctx, info, 0) != 0)
        {
            mbedtls_md_free(&kctx);
            return -1;
        }
        if (mbedtls_md_starts(&kctx) != 0 ||
            mbedtls_md_update(&kctx, key, key_len) != 0 ||
            mbedtls_md_finish(&kctx, k0) != 0)
        {
            mbedtls_md_free(&kctx);
            return -1;
        }
        mbedtls_md_free(&kctx);
    }
    else
    {
        (void)memcpy(k0, key, key_len);
    }

    for (size_t i = 0; i < block; ++i)
    {
        ipad[i] = k0[i] ^ 0x36;
        opad[i] = k0[i] ^ 0x5c;
    }

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    if (mbedtls_md_setup(&ctx, info, 0) != 0)
    {
        mbedtls_md_free(&ctx);
        return -1;
    }

    // inner = H(ipad || msg)
    if (mbedtls_md_starts(&ctx) != 0 ||
        mbedtls_md_update(&ctx, ipad, block) != 0 ||
        mbedtls_md_update(&ctx, msg, msg_len) != 0 ||
        mbedtls_md_finish(&ctx, tmp) != 0)
    {
        mbedtls_md_free(&ctx);
        return -1;
    }

    // outer = H(opad || inner)
    if (mbedtls_md_starts(&ctx) != 0 ||
        mbedtls_md_update(&ctx, opad, block) != 0 ||
        mbedtls_md_update(&ctx, tmp, dlen) != 0 ||
        mbedtls_md_finish(&ctx, out) != 0)
    {
        mbedtls_md_free(&ctx);
        return -1;
    }

    mbedtls_md_free(&ctx);
    return 0;
}

static mbedtls_md_context_t ctx;
static bool initialized = false;

void totp_init()
{
    if (initialized)
        return;

    mbedtls_md_init(&ctx);
    (void)mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    initialized = true;
}

bool totp_generate(const uint8_t* key, size_t key_len, uint64_t unix_time,
                   char out[7])
{
    if (!key || !out)
        return false;

    uint8_t msg[8];
    uint64_t steps = unix_time / 30ULL;
    for (int i = 7; i >= 0; --i)
    {
        msg[i] = (uint8_t)(steps & 0xFF);
        steps >>= 8;
    }

    uint8_t hash[20];
    if (hmac_sha1_md(key, key_len, msg, sizeof msg, hash) != 0)
        return false;

    uint8_t offset = hash[19] & 0x0F;
    uint32_t bin = ((hash[offset] & 0x7F) << 24) |
                   ((hash[offset + 1] & 0xFF) << 16) |
                   ((hash[offset + 2] & 0xFF) << 8) | (hash[offset + 3] & 0xFF);
    bin %= 1000000U;

    (void)snprintf(out, 7, "%06u", (unsigned)bin);
    return true;
}
