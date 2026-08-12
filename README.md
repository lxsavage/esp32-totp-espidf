# ESP32 TOTP (Espressif-IDF port)

An ESP32-based TOTP storing the private key in flash storage and getting time
synchronization information from the internet using NTP. This is based on my
[other TOTP firmware](https://github.com/lxsavage/esp32-totp) based on
ESP32-Arduino, which was based on
[this Arduino project](https://github.com/lxsavage/arduino-totp).

## Setup

> [!NOTE]
> This project was built with Espressif-IDF, on an ESP32 dev board.
> While other compatible configurations should work, they are
> currently untested due to me not having other hardware.

The setup of this system is as follows:

1. Wire the [hardware connections](#hardware-connections) as described below to
   the ESP32
2. Upload the program to the ESP32 and use [`load all`](#load) to write new
   credentials to memory (note that if you try to load into the main TOTP mode
   here, it will give you a bad config mode error)

> [!WARNING]
> The memory layout in this version is different than the ESP-Arduino version,
> so you will have to re-flash the key and WiFi credentials to memory with
> [`load all`](#load).

### Hardware Connections

> [!NOTE]
> This was written with a non-I2C wired LCD 1602, and does not currently
> support I2C-configured displays.

```plaintext
LCD 1602 (for display)

VSS -> GND
VDD -> 5V
V0  -> POT middle
RS  -> GPIO13
RW  -> GND
E   -> GPIO14
D4  -> GPIO26
D5  -> GPIO25
D6  -> GPIO18
D7  -> GPIO19
A   -> 220Ω resistor -> 5V
K   -> GND
```

```plaintext
POTENTIOMETER (for adjusting LCD contrast)

left -> GND
middle -> LCD V0
right -> 5V
```

```plaintext
BTN (for reset/entering load mode)

side 1 -> GPIO21
side 2 -> 10kΩ resistor -> 3.3V
```

> [!NOTE]
> These can be reconfigured under `components/common/config.h`

### Helper Utilities

> [!WARNING]
> These utilities were designed with POSIX-compatible systems in mind (i.e.,
> macOS and Linux-based distros). They will not work in their current form under
> Windows due to differences in how IO is handled at a low level there.

Requirements:

- C compiler (`cc` in path)
- Make (`make` in path)
- Firmware was compiled with `LOAD_MODE_ENABLED` set (see `components/common/config.h`)

Build with `make utilities` in `utilities/`.

#### Note on Device Name Used in Utilities

To find the board you want to interact with, do one of the following after connecting them:

1. (with arduino-cli) `arduino-cli board list` (will show up as Unknown board name, typically under something like `/dev/cu.usbserial-0001`)
2. (with ls) `ls /dev/cu.*`

The device identifier used in the utilities below is the `/dev/cu.`-prefixed device name

#### `load`

Load new secrets and/or wifi credentials. There are 3 subcommands with this,
`code`, `wifi`, and `all`.

- `code` - just set a new TOTP secret
   1. Enter credential loading mode (hold BTN down while resetting the board and
      release when "LOAD MODE" appears)
   2. `./utilities/load <devicename> code <base32 secret>`
   3. Board will reset on its own
- `wifi` - just set new wifi credentials
   1. Enter credential loading mode (hold BTN down while resetting the board
      and release when "LOAD MODE" appears), then press the button again after
      2 seconds.
   2. `./utilities/load <devicename> wifi <ssid> <ppk>`
   3. Board will reset on its own
- `all` - set a new code and set of wifi credentials
   1. Enter credential loading mode (hold BTN down while resetting the board
      and release when "LOAD MODE" appears)
   2. `./utilities/load <devicename> all <base32 secret> <ssid> <ppk>`
   3. Board will reset on its own

> [!TIP]
> For `code` and `all`, you can add the `--label <label>` flag to the end of the
> command to add a label to the TOTP code that will show up on boot while the
> time is being synchronized!

#### `clean`

~~Remove the secret/credential loaded from `load` securely in order to safely
reuse the ESP32 in other projects without worrying about leaking this
information. To invoke this utility, just use `./utilities/load <devicename>`.~~

**Unlike the ESP-Arduino version, this doesn't have a clean utility. Using
`idf.py erase-flash` achieves the same function. The only difference here is
that the firmware is also wiped when using the `erase-flash` command.**
