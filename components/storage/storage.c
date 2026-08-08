#include "storage.h"

#include <nvs_flash.h>

#define STORAGE_NVS_PART_NAME "nvs"

static bool initialized = false;
static nvs_handle_t handle;
void storage_init()
{
    if (initialized)
        return;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        // Reinit flash if full
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    else if (err != ESP_OK)
    {
        // Handle the error as normal otherwise
        ESP_ERROR_CHECK(err);
    }

    // Use PURGE so that old private data isn't recoverable
    ESP_ERROR_CHECK(
        nvs_open(STORAGE_NVS_PART_NAME, NVS_READWRITE_PURGE, &handle));

    initialized = true;
}

void storage_load_privatekey(struct storage_OTPCode* out)
{
    size_t* returned_size = NULL;

    ESP_ERROR_CHECK(nvs_get_blob(handle, "secret", out, returned_size));
    if (*returned_size != sizeof(*out))
    {
        // TODO - handle
    }
}

void storage_load_wifi(struct storage_WiFiDetails* out)
{
    size_t* returned_size = NULL;
    ESP_ERROR_CHECK(nvs_get_blob(handle, "secret", out, returned_size));
    if (*returned_size != sizeof(*out))
    {
        // TODO - handle
    }
}

void storage_write_secret(struct storage_OTPCode* in)
{
    ESP_ERROR_CHECK(nvs_set_blob(handle, "secret", in, sizeof(*in)));
}

void storage_write_wifi(struct storage_WiFiDetails* in)
{
    ESP_ERROR_CHECK(nvs_set_blob(handle, "wifi", in, sizeof(*in)));
}

_Bool storage_commit_writes()
{
    ESP_ERROR_CHECK(nvs_commit(handle));
    return true;
}
