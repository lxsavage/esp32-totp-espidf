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
#include "storage.h"

static struct storage_OTPCode decoded_key;
static struct storage_WiFiDetails wifi;

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

void app_main(void)
{
    serial_msg("BEGIN", NULL, NULL);

    display_init(RS, ENABLE, D4, D5, D6, D7);
    display_begin(16, 2);

    esp_sleep_enable_timer_wakeup(TOTP_POLL_NS);

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

    serial_msg("TIMESYNC", NULL, "wifi_ap_name");

    // totp::init();

    display_clear();
    display_set_cursor(0, 0);

    if (true) // decoded_key.label_len == 0)
    {
        display_write("Waiting for");
        display_set_cursor(0, 1);
        display_write("NTP sync...");
    }
    else
    {
        display_write("TOTP for");
        display_set_cursor(0, 1);
        // display_write(decoded_key.label);
    }

    // Delay at least LABEL_READ_TIME, but skip the delay if it took longer than
    // that to do a RTC sync
    int64_t sync_start_ts = esp_timer_get_time() / 1000;
    // bool last_sync_successful = rtc::sync(&network, true);

    if (false) // !rtc::ready() && !last_sync_successful)
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

    // Early return for no label; don't need to worry about reading a label!
    if (true) // decoded_key.label_len == 0)
    {
        display_clear();
        display_set_cursor(0, 1);
        display_write("Expires in   s");
        vTaskDelay(pdMS_TO_TICKS(100));
        return;
    }

    int64_t sync_end_ts = esp_timer_get_time() / 1000;
    if (sync_end_ts - sync_start_ts < LABEL_READ_TIME - 100)
        vTaskDelay(pdMS_TO_TICKS(LABEL_READ_TIME - 100 -
                                 (sync_end_ts - sync_start_ts)));

    display_clear();
    display_set_cursor(0, 1);
    display_write("Expires in   s");
    vTaskDelay(pdMS_TO_TICKS(100));
}
