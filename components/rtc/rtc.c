#include "rtc.h"

#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include <esp_timer.h>
#include <esp_wifi.h>

static uint64_t start_ts;
static uint64_t last_sync_microsec;

static _Bool wifi_initialized = false;

_Bool rtc_sync(struct storage_WiFiDetails* wifi, _Bool print_errors)
{
    uint64_t now = esp_timer_get_time();

    // Should skip processing if already set at least once, and
    // TIME_SYNC_INTERVAL has not been reached yet
    if (start_ts && (now - last_sync_microsec) / 1000 < TIME_SYNC_INTERVAL_SEC)
        return false;

    if (!wifi_initialized)
    {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_err_t err = esp_wifi_init(&cfg);
        if (err != ESP_OK)
        {
            ERR_CHECK(err);
            return false;
        }

        wifi_initialized = err == ESP_OK;
    }

    if (!wifi_initialized)
        return false;

    esp_err_t err = esp_wifi_start();
    ERR_CHECK(err);

    // do wifi stuff here

    err = esp_wifi_stop();
    ERR_CHECK(err);
    return err == ESP_OK;
}

_Bool rtc_ready()
{
    // Unlikely that we're running this at the UNIX epoch
    return start_ts != 0;
}

unsigned long rtc_get()
{
    // Account for the number of seconds since the last sync
    return (start_ts + (esp_timer_get_time() - last_sync_microsec)) / 1000000;
}
