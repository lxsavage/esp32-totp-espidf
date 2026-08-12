#include "storage.h"

#include <nvs_flash.h>

#include "helpers.h"

// This generally shouldn't change, so it's being left here instead of
// constants.h
#define STORAGE_NVS_PART_NAME "nvs"

static bool initialized = false;
static nvs_handle_t handle;

_Bool storage_init()
{
    if (initialized)
        return false;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        // Reinit flash if full
        ERR_CHECK(nvs_flash_erase());
        ERR_CHECK(nvs_flash_init());
    }
    else if (err != ESP_OK)
    {
#ifdef DEBUG
        ESP_ERROR_CHECK(err);
#endif
        return false;
    }

    // Use PURGE so that old private data isn't recoverable
    err = nvs_open(STORAGE_NVS_PART_NAME, NVS_READWRITE_PURGE, &handle);
#ifdef DEBUG
    ESP_ERROR_CHECK(err);
#endif

    initialized = err == ESP_OK;
    return initialized;
}

_Bool storage_load_privatekey(struct storage_OTPCode* out)
{
    size_t sz = sizeof(struct storage_OTPCode);
    esp_err_t err = nvs_get_blob(handle, "secret", out, &sz);
    if (err == ESP_ERR_NVS_NOT_FOUND)
        return false;
#ifdef DEBUG
    ESP_ERROR_CHECK(err);
#endif
    return err == ESP_OK && sz != sizeof(struct storage_OTPCode);
}

_Bool storage_load_wifi(struct storage_WiFiDetails* out)
{
    size_t sz = sizeof(struct storage_WiFiDetails);
    esp_err_t err = nvs_get_blob(handle, "wifi", out, &sz);
    if (err == ESP_ERR_NVS_NOT_FOUND)
        return false;
#ifdef DEBUG
    ESP_ERROR_CHECK(err);
#endif
    return err == ESP_OK && sz != sizeof(struct storage_WiFiDetails);
}

_Bool storage_write_secret(struct storage_OTPCode* in)
{
    esp_err_t err =
        nvs_set_blob(handle, "secret", in, sizeof(struct storage_OTPCode));
#ifdef DEBUG
    ESP_ERROR_CHECK(err);
#endif
    return err == ESP_OK;
}

_Bool storage_write_wifi(struct storage_WiFiDetails* in)
{
    esp_err_t err =
        nvs_set_blob(handle, "wifi", in, sizeof(struct storage_WiFiDetails));
#ifdef DEBUG
    ESP_ERROR_CHECK(err);
#endif
    return err == ESP_OK;
}

_Bool storage_commit_writes()
{
    esp_err_t err = nvs_commit(handle);
#ifdef DEBUG
    ESP_ERROR_CHECK(err);
#endif
    return err == ESP_OK;
}
