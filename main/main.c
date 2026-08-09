#include <driver/gpio.h>
#include <stdint.h>

#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_timer.h>

#include <freertos/FreeRTOS.h>

#include <freertos/projdefs.h>
#include <freertos/task.h>

#include "constants.h"

#include "display.h"
#include "rtc.h"
#include "storage.h"
#include "totp.h"

static struct storage_OTPCode decoded_key;
static struct storage_WiFiDetails wifi;

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

void totp_mode(void* _)
{
    esp_sleep_enable_timer_wakeup(TOTP_POLL_mS);
    for (;;)
    {
        unsigned long now = rtc_get();
        static char code_buf[7];
        if (!totp_generate(decoded_key.key, decoded_key.key_len, now, code_buf))
        {
            serial_msg("FAIL", "totp_gen", NULL);
            esp_light_sleep_start();
            return;
        }

        display_set_cursor(0, 0);
        for (int i = 0; i < 6; i++)
        {
            if (i == 3)
                display_write_byte(' ');
            display_write_byte(code_buf[i]);
        }

        static char exp_buf[3];
        snprintf(exp_buf, 3, "%02lu", 30 - (now % 30));
        display_set_cursor(11, 1);
        display_write(exp_buf);

        if (rtc_sync(&wifi, false))
            serial_msg("TIMESYNC", NULL, wifi.ssid);

        esp_light_sleep_start();
    }
}

void app_main(void)
{
#ifdef LOAD_TEST
#ifdef DEBUG
    test_harness();
    printf("Wrote test credentials; stalling\n");

    for (;;)
        vTaskDelay(pdMS_TO_TICKS(10000));
    return;
#endif
#endif

    serial_msg("BEGIN", NULL, NULL);

    display_init(RS, ENABLE, D4, D5, D6, D7);
    display_begin(16, 2);

    esp_sleep_enable_timer_wakeup(TOTP_POLL_mS);

    gpio_set_direction(LOAD_BTN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(LOAD_BTN, GPIO_PULLDOWN_ONLY);
    if (gpio_get_level(LOAD_BTN) == 1)
    {
        serial_msg("ENTER", "load", NULL);
        // load_mode
        serial_msg("EXIT", "load", NULL);
        serial_msg("RESTART", NULL, NULL);
        esp_restart();
    }
    else
        serial_msg("ENTER", "normal", NULL);

    storage_init();
    storage_load_wifi(&wifi);
    storage_load_privatekey(&decoded_key);

    serial_msg("TIMESYNC", NULL, wifi.ssid);

    totp_init();

    display_clear();
    display_set_cursor(0, 0);

    if (decoded_key.label_len == 0)
    {
        display_write("Waiting for");
        display_set_cursor(0, 1);
        display_write("NTP sync...");
    }
    else
    {
        display_write("TOTP for");
        display_set_cursor(0, 1);
        display_write(decoded_key.label);
    }

    // Delay at least LABEL_READ_TIME, but skip the delay if it took longer than
    // that to do a RTC sync
    int64_t sync_start_ts = esp_timer_get_time() / 1000;
    bool last_sync_successful = rtc_sync(&wifi, true);

    if (!rtc_ready() && !last_sync_successful)
    {
        // rtc::sync and rtc::ready both returning false indicates connection
        // failure
        display_clear();
        display_set_cursor(0, 0);
        display_write("ERROR");
        display_set_cursor(0, 1);
        display_write("Reconfigure WiFi");

        // Unrecoverable: lock until manual reset
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        esp_deep_sleep_start();
    }

    if (decoded_key.label_len != 0)
    {
        int64_t sync_end_ts = esp_timer_get_time() / 1000;
        if (sync_end_ts - sync_start_ts < LABEL_READ_TIME - 100)
            vTaskDelay(pdMS_TO_TICKS(LABEL_READ_TIME - 100 -
                                     (sync_end_ts - sync_start_ts)));
    }

    display_clear();
    display_set_cursor(0, 1);
    display_write("Expires in   s");
    vTaskDelay(pdMS_TO_TICKS(100));

    xTaskCreate(totp_mode, "totp_generator", 8192, NULL, 0, NULL);
}
