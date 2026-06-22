# ESP32 Battery OLED CC1101

PlatformIO/Arduino firmware for an ESP32 18650 board with an onboard 0.96"
128x64 I2C OLED and a 433 MHz CC1101 transceiver module.

This is the `module/cc1101` branch. The module-free board baseline stays on
`main`, while other hardware variants live on their own `module/*` branches.

The CC1101 branch is receive-only by default. It initializes a 433 MHz CC1101
over SPI, switches the radio to OOK/ASK direct receive mode, decodes common
433 MHz remote-control frames from `GDO0` with `rc-switch`, prints codes to
Serial Monitor, and renders sniffer state on the onboard OLED. No transmit path
is enabled in this firmware.

## Hardware

Target board:

- ESP32 OLED 18650 development board
- ESP32-WROOM-32
- Onboard OLED 0.96" 128x64 I2C
- USB, 5 V, or 18650 power
- 3.3 V GPIO logic

CC1101 module:

- Nettigo product: <https://nettigo.pl/products/modul-cc1101-transceiver-433-mhz-z-antena>
- CC1101 transceiver module for 433 MHz
- included 433 MHz antenna
- 2x4 THT header, 2.54 mm pitch
- supply voltage listed by Nettigo: `1.8 V - 3.6 V`

Use `3V3` only. Do not connect this module to 5 V.

## Pinout

CC1101 to ESP32:

| CC1101 pin number | CC1101 signal | ESP32 |
| ---: | --- | --- |
| 1 | GND | GND |
| 2 | Vcc | 3V3 |
| 3 | GD00 | GPIO26 |
| 4 | CSN | GPIO27 |
| 5 | SCK | GPIO18 |
| 6 | MOSI | GPIO23 |
| 7 | MISO/GD01 | GPIO19 |
| 8 | GD02 | GPIO25 |

Onboard OLED:

| OLED | ESP32 |
| --- | --- |
| SDA | GPIO5 |
| SCL | GPIO4 |

Pins to avoid for add-on modules unless a branch documents otherwise:

- `GPIO1` / `GPIO3`: USB serial and upload
- `GPIO6` - `GPIO11`: ESP32 flash
- `GPIO16`: onboard programmable LED on this board
- `GPIO0`, `GPIO2`, `GPIO12`, `GPIO15`: bootstrapping pins
- `GPIO34`, `GPIO35`, `GPIO36`, `GPIO39`: input only

## Build And Upload

```sh
pio run
pio run -t upload
pio device monitor -b 115200
```

The firmware uses:

- CC1101 frequency: `433.92 MHz`
- OOK/ASK bit rate hint: `4.8 kbps`
- receiver bandwidth: `270.0 kHz`
- direct data input: `GDO0=GPIO26`
- SPI: `SCK=GPIO18`, `MISO=GPIO19`, `MOSI=GPIO23`, `CSN=GPIO27`
- spare interrupt pin: `GDO2=GPIO25`
- OLED I2C: `SDA=GPIO5`, `SCL=GPIO4`
- Serial Monitor: `115200`

## Project Layout

```text
include/
  AppConfig.h          hardware pins, radio settings, timings and task config
  AppTasks.h           FreeRTOS task bootstrap
  Cc1101Service.h      CC1101 receive service API
  Cc1101Snapshot.h     thread-safe CC1101 data snapshot shape
  DiagnosticsLogger.h  Serial Monitor diagnostics API
  DisplayRenderer.h    OLED rendering API
src/
  AppTasks.cpp         task creation and task loops
  Cc1101Service.cpp
  DiagnosticsLogger.cpp
  DisplayRenderer.cpp
  main.cpp             Arduino setup/loop entrypoint
lib/
  U8g2/                local vendored OLED library
```

`Cc1101Service` owns the RadioLib `CC1101` instance and publishes a
`Cc1101Snapshot` behind a FreeRTOS mutex, so the OLED and diagnostics tasks
never read radio state while the receive task is updating decoded code data.

## FreeRTOS Tasks

| Task | Core | Priority | Period | Responsibility |
| --- | ---: | ---: | ---: | --- |
| `cc1101-rx` | 1 | 3 | 5 ms | Poll `rc-switch` decoded OOK/ASK codes |
| `oled-render` | 1 | 2 | 250 ms | Render boot, missing-radio, listening, or last-code screen |
| `serial-diag` | 0 | 1 | 2000 ms | Print structured diagnostic lines |

The Arduino `loop()` is intentionally idle and only calls `vTaskDelay()`.

## Libraries

U8g2 is stored in `lib/`, so the OLED driver builds from a local copy.

CC1101 support is pulled through PlatformIO:

```ini
lib_deps =
    jgromes/RadioLib @ 7.7.1
    sui77/rc-switch @ 2.6.4
```

## OLED Screens

At boot, the OLED shows the CC1101 module name, OOK/ASK mode, and the `GDO0`
capture pin.

If the radio is not detected, the OLED shows:

- `CC1101 FAIL`
- RadioLib state code
- SPI/power wiring hint
- last retry timestamp

When listening, the OLED shows:

- receive frequency
- current RSSI
- learned noise floor
- decoded code count

When a code is decoded, the OLED briefly shows:

- decimal value
- bit length
- protocol number
- pulse delay in microseconds
- repeat count
- last-code RSSI

## Serial Monitor

The Serial Monitor prints startup messages, radio initialization attempts, and
periodic diagnostic lines:

- radio presence
- listen state
- frequency
- RSSI and learned noise floor
- decoded code count
- last decimal value, bit length, protocol, pulse delay, repeat count, and binary value

Press a 433 MHz OOK/ASK remote button while the monitor is open. Useful decoded
lines look like this:

```text
[rcswitch] value=10597059 bits=24 protocol=1 delay=350us repeat=3 rssi=-34.0dBm binary=101000011011001011000011 raw=10920,340,1048,...
```

The `raw=` field is a short preview of the timing array returned by
`rc-switch`. It is printed to help compare CC1101 output with another 433 MHz
receiver.

## Radio Notes

This firmware is receive-only. Do not enable transmission unless you know the
legal frequency, bandwidth, duty-cycle, and output-power limits for your region
and use case.

The sniffer is tuned for common 433 MHz OOK/ASK remote controls such as socket
remotes. `rc-switch` recognizes common fixed-code protocols such as PT2262-like
and EV1527-like frames. It is a receive-only diagnostic tool, not a transmitter
or replay tool.

## Troubleshooting

CC1101 missing:

- Confirm `Vcc` is connected to `3V3`, not 5 V.
- Confirm `GND` is common with ESP32.
- Confirm SPI pins: `SCK=18`, `MISO=19`, `MOSI=23`, `CSN=27`.
- Confirm interrupt pins: `GD00=26`, `GD02=25`.
- Keep the antenna connected before using the radio.

No decoded codes:

- Confirm the transmitter is a 433 MHz OOK/ASK device.
- Try changing `Cc1101FrequencyMhz` in `include/AppConfig.h` to match the exact source.
- Try changing `Cc1101OokRxBandwidthKhz` between `203.0`, `270.0`, `325.0`, and `406.0`.
- Compare the `raw=` timing preview with a known-good 433 MHz receiver if one is available.
- Hold the remote close to the antenna for the first test.

OLED does not display:

- This project starts with `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`.
- If your board uses a SH1106 or another controller, switch the U8g2 constructor in `src/DisplayRenderer.cpp` to the matching `U8G2_SH1106_128X64_*_HW_I2C` variant.
- Confirm OLED pins `SDA=GPIO5` and `SCL=GPIO4` for your exact board revision.
- This board variant renders correctly with `U8G2_R2`, which rotates the OLED by 180 degrees. Use `U8G2_R0` if your display is mounted the other way.
