//
// parse.c
// Written by Logan Savage
//
// Read a raw TOTP URI and parse it for use with load utility
//

#include <stdbool.h>
#include <stdio.h>

#define MAX_URI_LEN 1024
#define TOTP_KEY_MAX 128
#define LABEL_LEN_MAX 16

// #define DEBUG

size_t read_uri(char* out, size_t out_len, FILE* fd)
{
    if (out_len == 0)
        return 0;

    // Ensure out buffer is always null-terminated if possible
    out[0] = '\0';
    if (out_len <= 1)
        return 0;

    (void)fgets(out, out_len, stdin);
    if (!out[0] || out[0] == '\n')
        return 0;

    size_t current_byte_i = 0;
    _Bool end_found = false;
    while (!end_found && current_byte_i < out_len)
    {
        switch (out[current_byte_i])
        {
        case '\r':
            if (current_byte_i >= out_len - 1 ||
                out[current_byte_i + 1] != '\n')
            {
                break;
            }
            // intentional fallthrough
        case '\n':
            out[current_byte_i] = '\0';
            // intentional fallthrough
        case '\0':
            end_found = true;
            break;
        default:
            current_byte_i++;
            break;
        }
    }

    // fgets will always either null-terminate overflow or leave
    // previously-defined null char in out[0], so it is safe to not redo it here

#ifdef DEBUG
    printf("uri[len %zu]=\"%s\"\turi[%zu]=0x%02X\n", current_byte_i - 1, out,
           current_byte_i, out[current_byte_i]);
#endif
    return current_byte_i;
}

_Bool parse_totp_uri(char* uri, size_t uri_len, char* out_secret,
                     size_t* out_secret_len, char* out_label,
                     size_t* out_label_len)
{
    if (uri_len == 0)
        return false;

    // Check if proper URI prefix is present
    const char PROTO[] = "otpauth://totp/";
    if (uri_len < sizeof(PROTO))
        return false;

    // (we can assume from previous check URI is longer than PROTO)
    size_t read_i = 0;
    while (read_i < sizeof(PROTO) - 1)
    {
        if (uri[read_i] != PROTO[read_i])
            return false;
        read_i++;
    }

    // Read label if present
    size_t label_len = 0;
    while (label_len < *out_label_len && read_i < uri_len && uri[read_i] != '?')
    {
        if (uri[read_i] == '%')
        {
            // Anything out of %XX is an invalid URI escape
            if (read_i + 2 >= uri_len)
                return false;

            // %20 maps to ' '
            if (uri[read_i + 1] == '2' && uri[read_i + 2] == '0')
            {
#ifdef DEBUG
                out_label[label_len++] = ' ';
#else
                out_label[label_len++] = '\b';
#endif
                read_i += 3;
            }
        }
        else
        {
            out_label[label_len++] = uri[read_i++];
        }
    }
    if (label_len >= *out_label_len)
        out_label[*out_label_len] = '\0';
    else
        out_label[label_len] = '\0';
    *out_label_len = label_len;

#ifdef DEBUG
    printf("label[len %zu]=\"%s\"", label_len, out_label);
#endif

    // Ensure secret is present
    const char SECRETPREF[] = "?secret=";
    size_t matched_chars = 0;

#ifdef DEBUG
    printf("uri=\"%s\", read_i=%zu\n", uri, read_i);
#endif
    while (matched_chars < sizeof(SECRETPREF) - 1)
    {
        if (read_i >= uri_len)
            return false;

        if (uri[read_i++] == SECRETPREF[matched_chars])
            matched_chars++;
        else
            matched_chars = 0;
    }

// Read the secret
#ifdef DEBUG
    printf("uri=\"%s\"\n\tread_i=%zu, uri[read_i]='%c'\n", uri, read_i,
           uri[read_i]);
#endif
    size_t secret_len = 0;
    while (secret_len < *out_secret_len && read_i < uri_len)
    {
        if (uri[read_i] == '&')
        {
            out_secret[secret_len++] = '\0';
            break;
        }

        out_secret[secret_len++] = uri[read_i++];
    }

    return true;
}

int main(int argc, char** argv)
{
    char buf[MAX_URI_LEN + 1];
    size_t uri_len = read_uri(buf, MAX_URI_LEN, stdin);

    if (uri_len == 0)
        return 1;

    char secret[TOTP_KEY_MAX];
    size_t secret_len = TOTP_KEY_MAX;
    char label[LABEL_LEN_MAX];
    size_t label_len = LABEL_LEN_MAX;

    if (!parse_totp_uri(buf, uri_len, secret, &secret_len, label, &label_len))
        return 2;

    printf("code %s", secret);
    if (label_len > 0)
        printf(" --label %s", label);

#ifdef DEBUG
    putchar('\n');
#endif
    return 0;
}
