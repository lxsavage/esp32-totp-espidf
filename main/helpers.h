#ifndef HELPERS_H_INCLUDED
#define HELPERS_H_INCLUDED

#include "constants.h"

#include <stdio.h>

void serial_msg(const char* command, const char* data, const char* rem)
{
    printf("%s", command);

    if (data != NULL)
        printf(" %s", data);

    if (rem != NULL)
        printf(" ;%s\n", rem);
    else
        printf("\n");
}

#ifdef LOAD_TEST
static struct storage_OTPCode code = {
    .label_len = 13,
    .key_len = 13,
    .label = "Hello, world!",
    .key = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!'}};

// TODO - fill this out
static struct storage_WiFiDetails creds = {.ppk = "", .ssid = ""};

void test_harness()
{
    storage_init();
    storage_write_secret(&code);
    storage_write_wifi(&creds);
    if (!storage_commit_writes())
        printf("failed to commit writes\n");
    else
    {
        storage_load_privatekey(&decoded_key);
        storage_load_wifi(&wifi);

        printf("Secret len: %u\n", decoded_key.key_len);
        printf("WiFi SSID: %s\n", wifi.ssid);
    }
}
#endif

#endif
