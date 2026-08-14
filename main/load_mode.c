#include "load_mode.h"

#include "config.h"
#include "constants.h"

#ifdef LOAD_MODE_ENABLED
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>

#include <driver/uart.h>
#include <esp_timer.h>
#include <freertos/task.h>

#include "display.h"
#include "helpers.h"
#include "storage.h"

// Initialize UART serial communication for input
static void uart_init(uart_port_t port)
{
    const uart_config_t cfg = {.baud_rate = UART_BAUD_RATE,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
                               .source_clk = UART_SCLK_DEFAULT};

    ERR_CHECK(uart_driver_install(port, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ERR_CHECK(uart_param_config(port, &cfg));
    ERR_CHECK(uart_set_pin(port, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
                           UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ERR_CHECK(uart_flush_input(port));
}

// Read a line from the specified UART serial port up until (but not including)
// '\n'. Use binary_mode to just keep reading until max_len characters are read.
//
// WARNING: binary_mode does not null-terminate the buffer!
static size_t uart_scan(uart_port_t port, char* buf, size_t max_len,
                        _Bool binary_mode)
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
                (void)uart_read_bytes(port, &next, 1, 0);
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
        (void)uart_read_bytes(port, &ch, 1, portMAX_DELAY);
        printf("%02x\n", ch);
    }
    else
    {
        // Ensure the buffer is properly ended with a null-terminator
        buf[(i < max_len) ? i : max_len - 1] = '\0';
    }

    return i;
}

void load_mode()
{
    struct storage_OTPCode code;
    struct storage_WiFiDetails network = {.ppk = {'\0'}, .ssid = {'\0'}};

    bool save_key = false;

    (void)storage_init();
    uart_init(UART_PORT);

    display_clear();
    display_set_cursor(0, 0);
    display_write("LOAD MODE    ...");
    display_set_cursor(0, 1);
    display_write("Reset to exit");

    // LOAD label //
    SERIAL_MSG("READY", "label", NULL);
    code.label_len = uart_scan(UART_PORT, code.label, 256, false);
    SERIAL_MSG("READ", "label", (const char*)code.label);

    // LOAD key length //
    SERIAL_MSG("READY", "key_len", NULL);
    size_t key_len;
    char keylen_buf[4];
    (void)uart_scan(UART_PORT, keylen_buf, 4, false);

    int len = atoi(keylen_buf);
    key_len = (size_t)len;
    SERIAL_MSG("READ", "key_len", keylen_buf);

    if (len < 0)
    {
        SERIAL_MSG("FAIL", "key_len", NULL);
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
        SERIAL_MSG("SKIP", "key", NULL);
        goto load_mode_network;
    }

    // LOAD key //

    SERIAL_MSG("READY", "key", NULL);
    code.key_len = uart_scan(UART_PORT, (char*)code.key, key_len, true);

    // This is okay to bypass the SERIAL_MSG macro due to this mode always
    // having it enabled
    printf("READ key ");
    for (size_t i = 0; i < code.key_len; i++)
        printf("%02x ", code.key[i]);
    printf("\n");

    save_key = true;
    (void)storage_write_secret(&code);

load_mode_network:
    // LOAD ssid //

    SERIAL_MSG("READY", "ssid", NULL);
    size_t ssid_len = uart_scan(UART_PORT, network.ssid, 32, false);

    // WORKAROUND - bug I haven't figured out yet with uart_getn that causes
    // the next string after a binary mode string is read to be empty
    if (ssid_len == 0)
        ssid_len = uart_scan(UART_PORT, network.ssid, 32, false);
    SERIAL_MSG("READ", "ssid", network.ssid);

    // Skip wifi if no SSID is provided
    if (ssid_len == 0)
    {
        if (save_key && !storage_commit_writes())
        {
            SERIAL_MSG("FAIL", "key", NULL);
            display_clear();
            display_set_cursor(0, 0);
            display_write("ERROR");
            display_set_cursor(0, 1);
            display_write("Data not saved");
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
        return;
    }

    // LOAD ppk //

    SERIAL_MSG("READY", "ppk", NULL);
    (void)uart_scan(UART_PORT, network.ppk, 64, false);
    SERIAL_MSG("READ", "ppk", network.ppk);

    // SAVE //

    if (save_key)
        SERIAL_MSG("SAVE", "key", NULL);

    (void)storage_write_wifi(&network);
    SERIAL_MSG("SAVE", "ssid", NULL);
    SERIAL_MSG("SAVE", "ppk", NULL);

    if (storage_commit_writes())
        return;

    SERIAL_MSG("FAIL", "key", NULL);
    SERIAL_MSG("FAIL", "ssid", NULL);
    SERIAL_MSG("FAIL", "ppk", NULL);

    display_clear();
    display_set_cursor(0, 0);
    display_write("ERROR");
    display_set_cursor(0, 1);
    display_write("Data not saved");

    vTaskDelay(pdMS_TO_TICKS(2000));
}
#endif
