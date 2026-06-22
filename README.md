# ESP32 Battery OLED CC1101

PlatformIO/Arduino firmware for an ESP32 18650 board with an onboard 0.96"
128x64 I2C OLED and a 433 MHz CC1101 transceiver module.

This is the `module/cc1101` branch. The module-free board baseline stays on
`main`, while other hardware variants live on their own `module/*` branches.

The CC1101 branch is receive-only by default. It initializes a 433 MHz CC1101
over SPI, listens for FSK packets, prints packet diagnostics to Serial Monitor,
and renders receiver state on the onboard OLED. No transmit path is enabled in
this firmware.

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

| CC1101 pin | ESP32 |
| --- | --- |
| GND | GND |
| Vcc | 3V3 |
| GD00 | GPIO26 |
| CSN | GPIO27 |
| SCK | GPIO18 |
| MOSI | GPIO23 |
| MISO/GD01 | GPIO19 |
| GD02 | GPIO25 |

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
- FSK bit rate: `4.8 kbps`
- frequency deviation: `5.0 kHz`
- receiver bandwidth: `125.0 kHz`
- SPI: `SCK=GPIO18`, `MISO=GPIO19`, `MOSI=GPIO23`, `CSN=GPIO27`
- interrupts: `GDO0=GPIO26`, `GDO2=GPIO25`
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
never read radio state while the receive task is updating packet data.

## FreeRTOS Tasks

| Task | Core | Priority | Period | Responsibility |
| --- | ---: | ---: | ---: | --- |
| `cc1101-rx` | 1 | 3 | 20 ms | Handle CC1101 receive interrupts and packet reads |
| `oled-render` | 1 | 2 | 500 ms | Render boot, missing-radio, listening, or packet screen |
| `serial-diag` | 0 | 1 | 2000 ms | Print structured diagnostic lines |

The Arduino `loop()` is intentionally idle and only calls `vTaskDelay()`.

## Libraries

U8g2 is stored in `lib/`, so the OLED driver builds from a local copy.

CC1101 support is pulled through PlatformIO:

```ini
lib_deps =
    jgromes/RadioLib @ 7.7.1
```

## OLED Screens

At boot, the OLED shows the CC1101 module name, receive-only mode, SPI pins, and
interrupt pins.

If the radio is not detected, the OLED shows:

- `CC1101 FAIL`
- RadioLib state code
- SPI/power wiring hint
- last retry timestamp

When listening, the OLED shows:

- receive frequency
- listen state
- packet count
- last RSSI
- CRC and receive error counters

When a packet is received, the OLED briefly shows:

- packet length
- RSSI and LQI
- packet count
- first bytes as hex preview

## Serial Monitor

The Serial Monitor prints startup messages, radio initialization attempts, and
periodic diagnostic lines:

- radio presence
- listen state
- frequency
- packet count
- CRC and receive error count
- RSSI and LQI
- last packet length and hex preview

## Radio Notes

This firmware is receive-only. Do not enable transmission unless you know the
legal frequency, bandwidth, duty-cycle, and output-power limits for your region
and use case.

The receiver only decodes packets using the same FSK settings as configured in
`include/AppConfig.h`. Many 433 MHz devices use OOK/ASK or proprietary packet
formats, so seeing only RSSI activity or no packets can be normal.

## Troubleshooting

CC1101 missing:

- Confirm `Vcc` is connected to `3V3`, not 5 V.
- Confirm `GND` is common with ESP32.
- Confirm SPI pins: `SCK=18`, `MISO=19`, `MOSI=23`, `CSN=27`.
- Confirm interrupt pins: `GD00=26`, `GD02=25`.
- Keep the antenna connected before using the radio.

No packets:

- Confirm the transmitter uses compatible FSK settings.
- Try changing `Cc1101FrequencyMhz` in `include/AppConfig.h` to match the exact source.
- Try changing bit rate, deviation, receiver bandwidth, and sync word if the source protocol is known.
- Remember that many remote controls and weather sensors use OOK/ASK, not this default FSK packet mode.

OLED does not display:

- This project starts with `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`.
- If your board uses a SH1106 or another controller, switch the U8g2 constructor in `src/DisplayRenderer.cpp` to the matching `U8G2_SH1106_128X64_*_HW_I2C` variant.
- Confirm OLED pins `SDA=GPIO5` and `SCL=GPIO4` for your exact board revision.
- This board variant renders correctly with `U8G2_R2`, which rotates the OLED by 180 degrees. Use `U8G2_R0` if your display is mounted the other way.
