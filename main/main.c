#include <driver/gpio.h>
#include <stddef.h>
#include <time.h>

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

#include "helpers.h"

static struct storage_OTPCode decoded_key;
static struct storage_WiFiDetails wifi;

// Not yet implemented; will uncomment later
#if FALSE
void load_mode()
{
    struct storage::OTPCode code;
    struct storage::WiFiDetails network = {.ppk = {'\0'}, .ssid = {'\0'}};

    bool save_key = false;

    storage::init();

    display::clear();
    display::set_cursor(0, 0);
    display::write("LOAD MODE    ...");
    display::set_cursor(0, 1);
    display::write("Reset to exit");

    // LOAD label //

    Serial.println("READY label");
    int64_t debounce_helper = esp_timer_get_time();
    while (!Serial.available())
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    code.label_len = Serial.readBytesUntil('\n', code.label, 255);
    code.label[code.label_len] = '\0';

    Serial.print("READ label ");
    Serial.println((char*)code.label);

    // LOAD key length //

    Serial.println("READY key_len");
    while (!Serial.available())
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (esp_timer_get_time() - debounce_helper > 2000000 &&
            digitalRead(LOAD_BTN) == HIGH)
        {
            Serial.println("SKIP label");
            Serial.println("SKIP key");
            goto load_mode_network;
        }
    }

    size_t key_len;
    {
        char key_len_chars[4];
        size_t key_len_chars_len =
            Serial.readBytesUntil('\n', key_len_chars, 4);
        key_len_chars[key_len_chars_len] = '\0';

        Serial.print("READ key_len ");
        Serial.println(key_len_chars);

        key_len = atoi(key_len_chars);

        if (key_len < 0)
        {
            Serial.println("FAIL key_len");
            display::clear();
            display::set_cursor(0, 0);
            display::write("ERROR");
            display::set_cursor(0, 1);
            display::write("Key not saved");
            vTaskDelay(pdMS_TO_TICKS(2000));
            return;
        }
        else if (key_len == 0)
        {
            Serial.println("SKIP key");
            goto load_mode_network;
        }
    }

    // LOAD key //

    Serial.println("READY key");
    while (!Serial.available())
        vTaskDelay(pdMS_TO_TICKS(10));

    {
        code.key_len = Serial.readBytes(code.key, key_len);

        // Read additional byte to remove/discard newline from buffer
        Serial.read();

        Serial.print("READ key ");
        for (size_t i = 0; i < code.key_len; i++)
        {
            Serial.print(code.key[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }

    save_key = true;
    storage::write_secret(code);

load_mode_network:
    // LOAD ssid //

    Serial.println("READY ssid");
    while (!Serial.available())
        vTaskDelay(pdMS_TO_TICKS(10));

    {
        size_t ssid_len = Serial.readBytesUntil('\n', network.ssid, 32);
        network.ppk[ssid_len] = '\0';
        Serial.print("READ ssid ");
        Serial.println(network.ssid);

        // Skip wifi if no SSID is provided
        if (ssid_len == 0)
        {
            if (save_key && !storage::commit_writes())
            {
                Serial.println("FAIL key");
                display::clear();
                display::set_cursor(0, 0);
                display::write("ERROR");
                display::set_cursor(0, 1);
                display::write("Data not saved");
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            return;
        }
    }

    // LOAD ppk //

    Serial.println("READY ppk");
    while (!Serial.available())
        vTaskDelay(pdMS_TO_TICKS(10));

    {
        size_t ppk_len = Serial.readBytesUntil('\n', network.ppk, 64);
        network.ppk[ppk_len] = '\0';

        Serial.print("READ ppk ");
        Serial.println(network.ppk);
    }

    // SAVE //

    if (save_key)
    {
        Serial.println("SAVE key");
    }

    storage::write_wifi(network);
    Serial.println("SAVE ssid");
    Serial.println("SAVE ppk");

    if (!storage::commit_writes())
    {
        Serial.println("FAIL key");
        Serial.println("FAIL ssid");
        Serial.println("FAIL ppk");
        display::clear();
        display::set_cursor(0, 0);
        display::write("ERROR");
        display::set_cursor(0, 1);
        display::write("Data not saved");
        vTaskDelay(pdMS_TO_TICKS(2000));
        return;
    }
}
#endif

void totp_mode(void* _)
{
    esp_sleep_enable_timer_wakeup(TOTP_POLL_mS);
    for (;;)
    {
        if (!rtc_ready())
        {
            serial_msg("TIMESYNC", NULL, wifi.ssid);

            display_set_cursor(0, 1);
            display_write("ReSync :RTC: ...");

            rtc_sync(&wifi, false);

            display_set_cursor(0, 1);
            display_write("Expires in   s  ");
        }

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

    // TODO - see if this is interfering with load mode entrance
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
    if (!rtc_ready() && !rtc_sync(&wifi, true))
    {
        // rtc_sync and rtc_ready both returning false indicates connection
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
        int64_t now = esp_timer_get_time() / 1000;
        if (now - sync_start_ts < LABEL_READ_TIME - 100)
            vTaskDelay(
                pdMS_TO_TICKS(LABEL_READ_TIME - 100 - (now - sync_start_ts)));
    }

    display_clear();
    display_set_cursor(0, 1);
    display_write("Expires in   s");
    vTaskDelay(pdMS_TO_TICKS(100));

    xTaskCreate(totp_mode, "totp_generator", 8192, NULL, 1, NULL);
}
