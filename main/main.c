#include <driver/gpio.h>
#include <stddef.h>
#include <time.h>

#include <esp_sleep.h>
#include <esp_system.h>
#include <esp_timer.h>

#include <freertos/FreeRTOS.h>

#include <freertos/projdefs.h>
#include <freertos/task.h>

#include "config.h"
#include "constants.h"

#include "display.h"
#include "rtc.h"
#include "storage.h"
#include "totp.h"

#include "helpers.h"
#include "load_mode.h"

static struct storage_OTPCode decoded_key;
static struct storage_WiFiDetails wifi;

void totp_loop(void* _ __attribute__((__unused__)))
{
    esp_sleep_enable_timer_wakeup(TOTP_POLL_uS);
    for (;;)
    {
        if (!rtc_ready())
        {
            SERIAL_MSG("TIMESYNC", NULL, wifi.ssid);

            display_set_cursor(0, 1);
            display_write("ReSync :RTC: ...");

            rtc_sync(&wifi, false);

            display_set_cursor(0, 1);
            display_write(TOTP_DISP_UNDERLAY);
        }

        unsigned long now = rtc_get();
        static char code_buf[7];
        if (!totp_generate(decoded_key.key, decoded_key.key_len, now, code_buf))
        {
            SERIAL_MSG("FAIL", "totp_gen", NULL);
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
        display_set_cursor(TOTP_DISP_EXP_START_COL, 1);
        display_write(exp_buf);

        esp_light_sleep_start();
    }
}

void app_main(void)
{
    SERIAL_MSG("BEGIN", NULL, NULL);

    (void)display_init(RS, ENABLE, D4, D5, D6, D7);
    (void)display_begin(16, 2);

#ifdef LOAD_MODE_ENABLED
    ERR_CHECK(gpio_set_direction(LOAD_BTN, GPIO_MODE_INPUT));
    if (gpio_get_level(LOAD_BTN) == 1)
    {
        SERIAL_MSG("ENTER", "load", NULL);
        load_mode();
        SERIAL_MSG("EXIT", "load", NULL);
#ifdef STALL_ON_LOAD_COMPLETE
        serial_msg("STALL", NULL NULL);
        return;
#else
        SERIAL_MSG("RESTART", NULL, NULL);
        esp_restart();
#endif
    }
    else
#endif
    {
        SERIAL_MSG("ENTER", "normal", NULL);
    }

    (void)totp_init();
    (void)storage_init();

    (void)storage_load_wifi(&wifi);
    (void)storage_load_privatekey(&decoded_key);

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

    SERIAL_MSG("TIMESYNC", NULL, wifi.ssid);
    int64_t sync_start_ts = esp_timer_get_time() / 1000;
    _Bool sync_evt_occurred = rtc_sync(&wifi, true);
    if (!rtc_ready() && !sync_evt_occurred)
    {
        // rtc_sync and rtc_ready both returning false indicates connection
        // failure
        display_clear();
        display_set_cursor(0, 0);
        display_write("ERROR");
        display_set_cursor(0, 1);
        display_write("Bad Config!");

        // Unrecoverable: lock until manual reset
        (void)esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        esp_deep_sleep_start();
        return;
    }

    if (decoded_key.label_len != 0)
    {
        // Delay at least LABEL_READ_TIME, but skip the delay if it took longer
        // than that to do a RTC sync
        int64_t now = esp_timer_get_time() / 1000;
        if (now - sync_start_ts < LABEL_READ_TIME_MS - 100)
            vTaskDelay(pdMS_TO_TICKS(LABEL_READ_TIME_MS - 100 -
                                     (now - sync_start_ts)));
    }

    // Write the TOTP display mask and trigger the codegen loop

    display_clear();
    display_set_cursor(0, 1);
    display_write(TOTP_DISP_UNDERLAY);
    vTaskDelay(pdMS_TO_TICKS(100));
    (void)xTaskCreate(totp_loop, "codegen_task", 8192, NULL, 1, NULL);
}
