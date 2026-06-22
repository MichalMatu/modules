# ESP32 Battery OLED MAX30100

PlatformIO/Arduino firmware for an ESP32 18650 board with an onboard 0.96"
128x64 I2C OLED and a MAX30100 pulse oximeter module.

This is the `module/max30100` branch. The module-free board baseline stays on
`main`, while other hardware variants live on their own `module/*` branches.

The MAX30100 branch reads heart-rate and SpO2 values over the shared I2C bus,
prints compact Serial Monitor diagnostics, and renders sensor state on the
onboard OLED with an expanded measurement view and a bottom status line.

This firmware is for hobby diagnostics and development. It is not a medical
device and its readings must not be used for health decisions.

## Board Reference

Target board: TTGO WiFi & Bluetooth Battery ESP32 0.96 Inch OLED Development Tool.

Product photos and board details below are from the
[HiTechChain product page](https://hitechchain.se/iot/ttgo/esp32-pico-kit-utvecklingsbord).

![TTGO ESP32 OLED 18650 board front](docs/images/ttgo-battery-oled-front.jpeg)

![TTGO ESP32 OLED 18650 board dimensions](docs/images/ttgo-battery-oled-dimensions.jpeg)

![TTGO ESP32 OLED 18650 board battery side](docs/images/ttgo-battery-oled-back.jpeg)

![TTGO ESP32 OLED board with OLED running](docs/images/ttgo-battery-oled-screen.jpeg)

![TTGO ESP32 OLED 18650 board pinout](docs/images/ttgo-battery-oled-pinout.jpeg)

![TTGO ESP32 OLED board alternate front view](docs/images/ttgo-battery-oled-front-screen.jpeg)

Short board specification:

- ESP32 development board with Wi-Fi and Bluetooth.
- ESP32-WROOM-32 module on the board revision shown in the product photos.
- Integrated 0.96" OLED.
- 18650 cell holder and onboard charging system.
- USB, 5 V, or 18650 power input paths.
- Charging and running at the same time are supported by the board.
- Charge indicator LED: red while charging, green when full.
- Power switch on the board.
- Extra programmable LED connected to `GPIO16` / `D0`.
- Listed charging current: 0.5 A.
- Listed output current: 1 A.
- Over-charge and over-discharge protection.
- Full ESP32 pin breakout.
- Product image marks approximate board dimensions as 114 mm x 28.25 mm x 17.6 mm.

## Hardware

Target board:

- ESP32 OLED 18650 development board
- ESP32-WROOM-32
- Onboard OLED 0.96" 128x64 I2C
- USB, 5 V, or 18650 power
- 3.3 V GPIO logic

MAX30100 module:

- MAX30100 pulse oximeter and heart-rate sensor
- red and IR LEDs with photodetector
- I2C interface
- default I2C address: `0x57`
- product reference: <https://sklep.msalamon.pl/produkt/czujnik-tetna-i-poziomu-tlenu-we-krwi-max30100/>

## Pinout

MAX30100 and onboard OLED share the same I2C bus:

| MAX30100 module | ESP32 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO5 |
| SCL | GPIO4 |
| INT | Not connected |

Onboard OLED:

| OLED | ESP32 |
| --- | --- |
| SDA | GPIO5 |
| SCL | GPIO4 |

Use `3V3` first. ESP32 GPIO is not 5 V tolerant, so do not use a breakout that
pulls `SDA` or `SCL` to 5 V unless you add level shifting. The linked product
page lists a wide power range, but also notes default logic at 1.8 V, so verify
the exact breakout revision before wiring it permanently.

Pins to avoid for add-on modules:

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

- MAX30100 I2C: address `0x57`
- shared I2C: `SDA=GPIO5`, `SCL=GPIO4`, `400 kHz`
- Serial Monitor: `115200`
- FreeRTOS tasks for MAX30100 polling, OLED rendering, and diagnostics

## Project Layout

```text
include/
  AppConfig.h             hardware pins, timings, task stack sizes and priorities
  AppTasks.h              FreeRTOS task bootstrap
  DiagnosticsLogger.h     Serial Monitor diagnostics API
  DisplayRenderer.h       OLED rendering API
  I2cBus.h                shared I2C bus lock/init API
  Max30100Service.h       MAX30100 service API
  Max30100Snapshot.h      thread-safe MAX30100 data snapshot shape
src/
  AppTasks.cpp            task creation and task loops
  DiagnosticsLogger.cpp
  DisplayRenderer.cpp
  I2cBus.cpp
  Max30100Service.cpp
  main.cpp                Arduino setup/loop entrypoint
lib/
  U8g2/                   local vendored OLED library
```

`Max30100Service` owns `PulseOximeter` from `MAX30100lib` and publishes a
`Max30100Snapshot` behind a FreeRTOS mutex, so the OLED and diagnostics tasks
never read sensor internals while the polling task is updating the sample
pipeline.

`I2cBus` initializes the shared ESP32 I2C bus and serializes OLED and MAX30100
transactions from separate FreeRTOS tasks.

## FreeRTOS Tasks

| Task | Core | Priority | Period | Responsibility |
| --- | ---: | ---: | ---: | --- |
| `max30100` | 1 | 3 | 5 ms | Poll MAX30100 and publish readings |
| `oled-render` | 1 | 2 | 500 ms | Render boot, missing-sensor, waiting, or measurement screen |
| `serial-diag` | 0 | 1 | 2000 ms | Print structured diagnostic lines |

The Arduino `loop()` is intentionally idle and only calls `vTaskDelay()`.

## Libraries

U8g2 is stored in `lib/`, so the OLED driver builds from a local copy.

MAX30100 support is pulled through PlatformIO:

```ini
lib_deps =
    oxullo/MAX30100lib @ ^1.2.1
```

The library's `PulseOximeter` helper provides beat detection, heart-rate, and
SpO2 calculation. A value of `0` means the reading is not valid yet.

## OLED Screens

At boot, the OLED shows a full-screen finger-over-sensor prompt icon.

If the sensor is not detected, the OLED shows:

- `MAX30100 FAIL`
- I2C/power wiring hint
- address `0x57`
- last retry timestamp

If the sensor is detected but readings are not valid yet, the OLED shows:

- a full-screen finger-over-sensor prompt icon

When readings are available, the OLED shows:

- large BPM and SpO2 values side by side
- recent beat indicator on the bottom status line
- beat count on the bottom status line
- current red LED bias index on the bottom status line

The measurement screen uses the bottom OLED line for compact status data.

## Serial Monitor

The Serial Monitor prints startup messages, sensor detection attempts, and
periodic diagnostic lines:

- sensor presence
- BPM and validity flag
- SpO2 and validity flag
- recent beat flag
- beat count
- red LED current bias index

## Troubleshooting

MAX30100 missing:

- Confirm `VCC` is connected to `3V3` and `GND` to `GND`.
- Confirm `SDA=GPIO5` and `SCL=GPIO4`.
- Scan the bus if another I2C address is suspected; this firmware expects `0x57`.
- Verify that pull-ups on the MAX30100 breakout do not pull the bus to 5 V.
- Keep a stable finger contact on the optical window; no reading is expected in open air.

No stable BPM or SpO2:

- Keep the finger still and cover both LEDs and the detector.
- Avoid strong ambient light hitting the sensor.
- Wait several seconds for the beat detector and SpO2 calculator to stabilize.
- Try lowering or raising LED current in `src/Max30100Service.cpp` if your breakout saturates or reads weakly.

OLED does not display:

- This project starts with `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`.
- If your board uses a SH1106 or another controller, switch the U8g2 constructor in `src/DisplayRenderer.cpp` to the matching `U8G2_SH1106_128X64_*_HW_I2C` variant.
- Confirm OLED pins `SDA=GPIO5` and `SCL=GPIO4` for your exact board revision.
- This board variant renders correctly with `U8G2_R2`, which rotates the OLED by 180 degrees. Use `U8G2_R0` if your display is mounted the other way.
