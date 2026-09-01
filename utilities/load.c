//
// load.c
// Written by Logan Savage
//
// Load new secrets and WiFi credentials on to an ESP32 with the main ESP32-TOTP
// firmware loaded on it which is in load mode.
//

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "helpers/base32.h"
#include "helpers/serial.h"

#define SERIAL_BUF_LEN 256
#define TOTP_KEY_MAX 128

// Uncomment this to get debug logging
// #define DEBUG_LOGS

// Handles sending the proper secret over serial
_Bool handle_secret(int serial_fd, const char* secret, const char* label,
                    size_t secret_len, size_t label_len)
{
    if (secret_len >= SERIAL_BUF_LEN)
    {
        fprintf(stderr,
                "secret is too long: max chars is %d, secret is %lu "
                "chars.\n",
                SERIAL_BUF_LEN, secret_len);
        return false;
    }

    if (!base32valid(secret))
        fprintf(stderr, "warning: secret is not base32\n");

    char secret_dec[TOTP_KEY_MAX];
    size_t secret_dec_len =
        base32decode(secret, (unsigned char*)secret_dec, TOTP_KEY_MAX);

    if (label_len > 0)
    {
#ifdef DEBUG_LOGS
        fprintf(stderr, "send serial (fd: %d, len: %lu): %s\n", serial_fd,
                label_len, label);
#endif
        if (!send_serial(serial_fd, label, label_len))
        {
            fprintf(stderr, "failed to send label");
            return false;
        }
    }
    else
    {
#ifdef DEBUG_LOGS
        fprintf(stderr, "No label found; skipping send...\n");
        fprintf(stderr, "send serial (fd: %d, len: %d) bin: (null)\n",
                serial_fd, 0);
#endif
        if (!send_serial(serial_fd, NULL, 0))
        {
            fprintf(stderr, "failed to send label skip message");
            return false;
        }
    }

    char len_str[4];
    snprintf(len_str, 4, "%zu", secret_dec_len);

#ifdef DEBUG_LOGS
    fprintf(stderr, "secret code (base32 encoded): %s\n", secret);
    fprintf(stderr, "send serial (fd: %d, len: %lu): %s\n", serial_fd,
            strlen(len_str), len_str);
    fprintf(stderr, "send serial (fd: %d, len: %lu) bin: ", serial_fd,
            secret_dec_len);

    for (int i = 0; i < secret_dec_len; i++)
    {
        fprintf(stderr, "%02x ", (uint8_t)secret_dec[i]);
    }
    fprintf(stderr, "\n");
#endif

    if (!send_serial(serial_fd, len_str, strlen(len_str)))
    {
        fprintf(stderr, "failed to send secret length");
        return false;
    }

    if (!send_serial(serial_fd, secret_dec, secret_dec_len))
    {
        fprintf(stderr, "failed to send secret");
        return false;
    }
    return true;
}

// Handles sending the WiFi credentials properly over serial. Returns true if
// successful, false otherwise.
_Bool handle_wifi(int serial_fd, const char* ssid, const char* ppk,
                  size_t ssid_len, size_t ppk_len)
{

    if (ssid_len > 32)
    {
        fprintf(stderr,
                "SSID is too long: max chars is 32, SSID is %lu chars.\n",
                ssid_len);
        return false;
    }
    if (ppk_len > 64)
    {
        fprintf(stderr, "PPK is too long: max chars is 64, PPK is %lu chars.\n",
                ppk_len);
        return false;
    }

#ifdef DEBUG_LOGS
    fprintf(stderr, "send serial (fd: %d, len: %lu): %s\n", serial_fd, ssid_len,
            ssid);
#endif
    if (!send_serial(serial_fd, ssid, ssid_len))
    {
        fprintf(stderr, "failed to load SSID\n");
        return false;
    }

#ifdef DEBUG_LOGS
    fprintf(stderr, "send serial (fd: %d, len: %lu): %s\n", serial_fd, ppk_len,
            ppk);
#endif
    if (!send_serial(serial_fd, ppk, ppk_len))
    {
        fprintf(stderr, "failed to load PPK\n");
        return false;
    }
    return true;
}

// Handles cleaning up space placeholder '\b' that is used in parse utility for
// labels
void cleanup_parseplaceholders(char* label_raw)
{
    for (size_t i = 0; i < strlen(label_raw); i++)
    {
        if (label_raw[i] == '\b')
            label_raw[i] = ' ';
    }
}
int main(int argc, const char* argv[])
{
    if (argc < 4)
    {
        fprintf(stderr,
                "Usage: %s <device> <code | wifi | all> <data> [--label "
                "<label>]\n",
                argv[0]);
        fprintf(stderr,
                "- %s <device> code <base32 secret> [--label <label>]\n",
                argv[0]);
        fprintf(stderr, "- %s <device> wifi <ssid> <ppk>\n", argv[0]);
        fprintf(stderr,
                "- %s <device> all <base32 secret> <ssid> <ppk> [--label "
                "<label>]\n",
                argv[0]);

        goto exit_bad;
    }

    int fd;
    if ((fd = open_port(argv[1])) < 0)
    {
        fprintf(stderr, "failed to open serial port\n");
        goto exit_bad;
    }

    char label[16];
    size_t label_len = 0;
    if (argc >= 3 && strncmp("--label", argv[argc - 2], 7) == 0)
    {
        label_len = snprintf(label, 16, "%s", argv[argc - 1]);
#ifdef DEBUG_LOGS
        fprintf(stderr, "label: %s\n", label);
#endif
    }
#ifdef DEBUG_LOGS
    else if (strcmp(argv[2], "wifi") != 0)
        fprintf(stderr, "no label found, just sending raw secret\n");
#endif

    // This is used for storing a parsed secret from the code-only section; it's
    // declared here to avoid compiler errors from the free call further down,
    // which also will only be reached if the code-only branch runs with an
    // error
    const char* secret = NULL;
    if (strcmp(argv[2], "code") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "Usage:\n%s <device> code <base32 secret>\n",
                    argv[0]);
            goto exit_bad_close;
        }

        // Ensure that the secret cannot be changed after parsing
        size_t arg3_len = strlen(argv[3]);
        char* secret_raw = malloc(arg3_len + 1);
        snprintf(secret_raw, arg3_len + 1, "%s", argv[3]);
        cleanup_parseplaceholders(secret_raw);
        secret = (const char*)secret_raw;

        size_t secret_len = strlen(secret);

        if (secret_len >= 256)
        {
            fprintf(
                stderr,
                "secret is too long: max chars is 256, secret is %lu chars.\n",
                secret_len);
            goto exit_bad_close_withcode;
        }
        if (!handle_secret(fd, secret, label, secret_len, label_len))
        {
            fprintf(stderr, "failed to load code");
            goto exit_bad_close_withcode;
        }
        free((void*)secret);

        // Skip the wifi loading
#ifdef DEBUG_LOGS
        fprintf(stderr, "send serial (fd: %d, len: %lu) bin: ", fd, 0);
#endif
        send_serial(fd, "", 0);
    }
    else if (strcmp(argv[2], "wifi") == 0)
    {
        if (argc < 4)
        {
            fprintf(stderr, "Usage:\n%s <device> wifi <ssid> <ppk>\n", argv[0]);
            goto exit_bad_close;
        }

        // Skip the code loading
        send_serial(fd, "", 0);

        const char* ssid = argv[3];
        const char* ppk = argv[4];
        size_t ssid_len = strlen(ssid);
        size_t ppk_len = strlen(ppk);

        if (!handle_wifi(fd, ssid, ppk, ssid_len, ppk_len))
        {
            fprintf(stderr, "failed to load WiFi information\n");
            goto exit_bad_close;
        }
    }
    else if (strcmp(argv[2], "all") == 0)
    {
        if (argc < 5)
        {
            fprintf(stderr,
                    "Usage:\n%s <device> all <base32 secret> <ssid> <ppk>\n",
                    argv[0]);
            goto exit_bad_close;
        }

        const char* secret = argv[3];
        const char* ssid = argv[4];
        const char* ppk = argv[5];
        size_t secret_len = strlen(secret);
        size_t ssid_len = strlen(ssid);
        size_t ppk_len = strlen(ppk);

        const char* label;
        _Bool has_label = false;
        for (int i = 0; i < argc - 1; i++)
        {
            if (strcmp("--label", argv[i]) == 0)
            {
                label = argv[i + 1];
                has_label = true;
                break;
            }
        }

        if (!handle_secret(fd, secret, label, secret_len, label_len))
        {
            fprintf(stderr, "failed to load secret\n");
            goto exit_bad_close;
        }

        if (!handle_wifi(fd, ssid, ppk, ssid_len, ppk_len))
        {
            fprintf(stderr, "failed to load WiFi information\n");
            goto exit_bad_close;
        }
    }
    else
    {
        fprintf(stderr, "Unknown command %s\n", argv[2]);
        goto exit_bad_close;
    }

    // Uneventful exit
    close(fd);
    printf("Successfully wrote data to %s\n", argv[1]);
    return 0;

exit_bad_close_withcode:
    // Free the secret allocation made specifically for the escape case from
    // parse.c
    free((void*)secret);
exit_bad_close:
    close(fd);
exit_bad:
    return 1;
}
