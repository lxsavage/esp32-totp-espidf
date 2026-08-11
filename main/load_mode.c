#include "load_mode.h"

#include "constants.h"

#ifndef LOAD_MODE_ENABLED
void load_mode()
{
    // noop
}
#else
#include <string.h>

#include <freertos/FreeRTOS.h>

#include <driver/uart.h>
#include <esp_timer.h>
#include <freertos/task.h>

#include "display.h"
#include "helpers.h"
#include "storage.h"

#define UART_PORT UART_NUM_0
#define UART_BUF_SIZE 256

static void uart_init(uart_port_t port)
{
    const uart_config_t cfg = {.baud_rate = 115200,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                               .source_clk = UART_SCLK_DEFAULT};

    esp_err_t err;

    err = uart_driver_install(port, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    ERR_CHECK(err);

    err = uart_param_config(port, &cfg);
    ERR_CHECK(err);

    err = uart_set_pin(port, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    ERR_CHECK(err);

    uart_flush_input(port);
}

size_t uart_getn(uart_port_t port, char* buf, size_t max_len, _Bool binary_mode)
{
    size_t i = 0;

    if (binary_mode)
    {
        while (i < max_len)
        {
            uint8_t ch;
            int n = uart_read_bytes(port, &ch, 1, portMAX_DELAY);
            if (n <= 0)
                continue;
            buf[i++] = (char)ch;
        }
        return i; // exact number of binary bytes read
    }

    while (i < max_len - 1)
    {
        uint8_t ch;
        int n = uart_read_bytes(port, &ch, 1, portMAX_DELAY);
        if (n <= 0)
            continue;
        if (ch == '\n' || ch == '\r')
        {
            if (ch == '\r')
            {
                uint8_t next;
                uart_read_bytes(port, &next, 1, 0);
            }
            break;
        }
        buf[i++] = (char)ch;
    }

    if (binary_mode)
    {
        // WORKAROUND - in binary mode, explicitly discard the null terminator
        // instead of just stopping with it remaining in the case that would
        // happen if a buffer overflow is blocked in non-binary mode
        uint8_t ch;
        uart_read_bytes(port, &ch, 1, portMAX_DELAY);
        printf("%02x\n", ch);
    }
    else
    {
        buf[i] = '\0';
    }

    return i;
}

void load_mode()
{
    struct storage_OTPCode code;
    struct storage_WiFiDetails network = {.ppk = {'\0'}, .ssid = {'\0'}};

    bool save_key = false;

    storage_init();
    uart_init(UART_PORT);

    display_clear();
    display_set_cursor(0, 0);
    display_write("LOAD MODE    ...");
    display_set_cursor(0, 1);
    display_write("Reset to exit");

    // LOAD label //
    serial_msg("READY", "label", NULL);
    code.label_len = uart_getn(UART_PORT, code.label, 256, false);
    printf("READ label %s\n", (char*)code.label);

    // LOAD key length //

    serial_msg("READY", "key_len", NULL);
    size_t key_len;
    {
        char discard[4];
        uart_getn(UART_PORT, discard, 4, false);

        printf("READ key_len %s\n", discard);
        int len = atoi(discard);
        key_len = (size_t)len;

        if (len < 0)
        {
            serial_msg("FAIL", "key_len", NULL);
            display_clear();
            display_set_cursor(0, 0);
            display_write("ERROR");
            display_set_cursor(0, 1);
            display_write("Key not saved");
            vTaskDelay(pdMS_TO_TICKS(2000));
            return;
        }
        else if (key_len == 0)
        {
            serial_msg("SKIP", "key", NULL);
            goto load_mode_network;
        }
    }

    // LOAD key //

    serial_msg("READY", "key", NULL);
    {
        code.key_len = uart_getn(UART_PORT, (char*)code.key, key_len, true);

        printf("READ key ");
        for (size_t i = 0; i < code.key_len; i++)
        {
            printf("%02x ", code.key[i]);
        }
        printf("\n");
    }

    save_key = true;
    storage_write_secret(&code);

load_mode_network:
    // LOAD ssid //

    serial_msg("READY", "ssid", NULL);
    {
        size_t ssid_len = uart_getn(UART_PORT, network.ssid, 32, false);

        // WORKAROUND - bug I haven't figured out yet with uart_getn that causes
        // the next string after a binary mode string is read to be empty
        if (ssid_len == 0)
            ssid_len = uart_getn(UART_PORT, network.ssid, 32, false);
        serial_msg("READ", "ssid", network.ssid);

        // Skip wifi if no SSID is provided
        if (ssid_len == 0)
        {
            if (save_key && !storage_commit_writes())
            {
                serial_msg("FAIL", "key", NULL);
                display_clear();
                display_set_cursor(0, 0);
                display_write("ERROR");
                display_set_cursor(0, 1);
                display_write("Data not saved");
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            return;
        }
    }

    // LOAD ppk //

    serial_msg("READY", "ppk", NULL);
    {
        uart_getn(UART_PORT, network.ppk, 64, false);
        serial_msg("READ", "ppk", network.ppk);
    }

    // SAVE //

    if (save_key)
        serial_msg("SAVE", "key", NULL);

    storage_write_wifi(&network);
    serial_msg("SAVE", "ssid", NULL);
    serial_msg("SAVE", "ppk", NULL);

    if (storage_commit_writes())
        return;

    serial_msg("FAIL", "key", NULL);
    serial_msg("FAIL", "ssid", NULL);
    serial_msg("FAIL", "ppk", NULL);

    display_clear();
    display_set_cursor(0, 0);
    display_write("ERROR");
    display_set_cursor(0, 1);
    display_write("Data not saved");

    vTaskDelay(pdMS_TO_TICKS(2000));
}
#endif
