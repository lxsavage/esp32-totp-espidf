#include "rtc.h"

#include <string.h>
#include <time.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"

#include "freertos/event_groups.h"

#include "storage.h"

#define WIFI_CONNECT_TIMEOUT_MS 30000
#define SNTP_WAIT_TIMEOUT_MS 20000
#define RTC_VALID_EPOCH 1546300800UL // 2019-01-01

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char* TAG = "rtc_time";

static EventGroupHandle_t s_wifi_evtgrp;
static bool s_wifi_driver_inited = false;
static bool s_event_loop_inited = false;
static bool s_netif_inited = false;
static bool s_time_ready = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        xEventGroupSetBits(s_wifi_evtgrp, WIFI_FAIL_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(s_wifi_evtgrp, WIFI_CONNECTED_BIT);
    }
}

static void copy_ensure(char* dst, size_t dstsz, const char* src)
{
    if (!dst || dstsz == 0)
        return;
    if (!src)
    {
        dst[0] = '\0';
        return;
    }
    size_t n = strnlen(src, dstsz - 1);
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static bool ensure_bases(bool log)
{
    esp_err_t err;

    if (!s_event_loop_inited)
    {
        err = esp_event_loop_create_default();
        if (err == ESP_OK || err == ESP_ERR_INVALID_STATE)
        {
            s_event_loop_inited = true;
        }
        else
        {
            if (log)
                ESP_LOGE(TAG, "Event loop init failed: %s",
                         esp_err_to_name(err));
            return false;
        }
    }

    if (!s_netif_inited)
    {
        err = esp_netif_init();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        {
            if (log)
                ESP_LOGE(TAG, "esp_netif_init failed: %s",
                         esp_err_to_name(err));
            return false;
        }
        esp_netif_create_default_wifi_sta();
        s_netif_inited = true;
    }

    if (!s_wifi_evtgrp)
        s_wifi_evtgrp = xEventGroupCreate();
    return true;
}

static bool wifi_start_connect(const struct storage_WiFiDetails* wifi, bool log)
{
    if (!ensure_bases(log))
        return false;

    if (!s_wifi_driver_inited)
    {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_err_t err = esp_wifi_init(&cfg);
        if (err != ESP_OK)
        {
            if (log)
                ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(err));
            return false;
        }
        s_wifi_driver_inited = true;

        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_register(
            WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_instance_register(
            IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    }

    wifi_config_t wcfg = {0};
    wcfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wcfg.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    copy_ensure((char*)wcfg.sta.ssid, sizeof(wcfg.sta.ssid), wifi->ssid);
    copy_ensure((char*)wcfg.sta.password, sizeof(wcfg.sta.password), wifi->ppk);

    if (wcfg.sta.password[0] == '\0')
    {
        wcfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK)
    {
        if (log)
            ESP_LOGE(TAG, "set mode failed: %s", esp_err_to_name(err));
        return false;
    }
    err = esp_wifi_set_config(WIFI_IF_STA, &wcfg);
    if (err != ESP_OK)
    {
        if (log)
            ESP_LOGE(TAG, "set config failed: %s", esp_err_to_name(err));
        return false;
    }
    err = esp_wifi_start();
    if (err != ESP_OK)
    {
        if (log)
            ESP_LOGE(TAG, "wifi start failed: %s", esp_err_to_name(err));
        return false;
    }

    xEventGroupClearBits(s_wifi_evtgrp, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_evtgrp, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE,
        pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT)
    {
        if (log)
            ESP_LOGI(TAG, "Wi-Fi connected");
        return true;
    }

    if (log)
        ESP_LOGW(TAG, "Wi-Fi connect timeout/failure");
    return false;
}

static void wifi_stop_deinit(bool log)
{
    if (s_wifi_driver_inited)
    {
        esp_err_t err = esp_wifi_stop();
        if (err != ESP_OK && log)
        {
            ESP_LOGW(TAG, "wifi stop: %s", esp_err_to_name(err));
        }
        err = esp_wifi_deinit();
        if (err != ESP_OK && log)
        {
            ESP_LOGW(TAG, "wifi deinit: %s", esp_err_to_name(err));
        }
        s_wifi_driver_inited = false;
    }
}

static bool time_is_valid(void)
{
    time_t now = 0;
    time(&now);
    return (unsigned long)now >= RTC_VALID_EPOCH;
}

static bool sntp_sync_once(bool log)
{
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    uint32_t waited = 0;
    const uint32_t step_ms = 500;

    while (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET &&
           waited < SNTP_WAIT_TIMEOUT_MS)
    {
        vTaskDelay(pdMS_TO_TICKS(step_ms));
        waited += step_ms;
    }

    bool ok = time_is_valid();

    if (ok && log)
    {
        time_t now = 0;
        struct tm tm = {0};
        time(&now);
        localtime_r(&now, &tm);
        ESP_LOGI(TAG, "Time synced: %04d-%02d-%02d %02d:%02d:%02d",
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
                 tm.tm_min, tm.tm_sec);
    }
    else if (!ok && log)
    {
        ESP_LOGW(TAG, "SNTP sync timeout or invalid time");
    }

    esp_sntp_stop();
    return ok;
}

_Bool rtc_sync(struct storage_WiFiDetails* wifi, _Bool print_errors)
{
    // Not thread-safe by contract
    if (!wifi || wifi->ssid[0] == '\0')
    {
        if (print_errors)
            ESP_LOGE(TAG, "Invalid Wi-Fi details");
        return false;
    }

    bool ok = false;

    if (wifi_start_connect(wifi, print_errors))
        ok = sntp_sync_once(print_errors);

    wifi_stop_deinit(print_errors);

    // Mark ready if system time looks sane
    s_time_ready = ok || time_is_valid();
    return s_time_ready;
}

_Bool rtc_ready(void)
{
    // Thread-safe read; also cross-check system time for robustness
    if (s_time_ready)
        return true;
    if (time_is_valid())
    {
        s_time_ready = true;
        return true;
    }
    return false;
}

unsigned long rtc_get(void)
{
    time_t now = 0;
    time(&now);
    return (unsigned long)now;
}
