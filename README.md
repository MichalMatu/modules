# ESP32 Module Base

PlatformIO/Arduino base firmware for ESP32-family module experiments in VS Code.
`main` is the shared starting point for future ESP32, ESP32-S3, and related
board profiles. Concrete sensors and peripheral builds belong on module
branches.

The baseline intentionally stays module-free. It provides:

- a small Arduino entrypoint
- a board hardware profile
- OLED initialization and a simple status screen
- a multi-level logger
- memory and stack diagnostics
- explicit FreeRTOS task wiring
- local vendored libraries for reproducible builds

Current module branches:

- `module/gps`: u-blox NEO-M8M GPS over UART2 with TinyGPSPlus parsing and OLED diagnostics.

## Board Reference

The active hardware profile targets a TTGO WiFi & Bluetooth Battery ESP32 0.96
Inch OLED development board. To adapt the baseline to another ESP32-family
board, edit or replace `include/HardwareProfile.h`.

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

Active profile:

| Feature | ESP32 |
| --- | --- |
| OLED SDA | GPIO5 |
| OLED SCL | GPIO4 |
| OLED rotation | `U8G2_R2` |

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

Run all local regression checks before handing off changes:

```sh
scripts/checks.sh
```

Install the repository pre-commit hook once per clone:

```sh
scripts/install_hooks.sh
```

`platformio.ini` uses shared `[env]` settings plus a concrete default
`[env:esp32dev]` target. Add future ESP32-S3 or board-specific environments as
small `[env:<board>]` sections that inherit the shared flags.

The shared environment sets:

- `framework = arduino`
- `-std=gnu++17`
- `APP_LOG_LEVEL=2` for info-level logging by default
- `CORE_DEBUG_LEVEL=0` to keep Arduino core debug logs out of the base build
- `build_src_flags` with `-fno-exceptions` and `-fno-rtti` for project source
- local library discovery from `lib/`

The active hardware profile uses OLED I2C `SDA=GPIO5`, `SCL=GPIO4`.

VS Code workspace recommendations are intentionally minimal and tracked:

- `.vscode/extensions.json` recommends PlatformIO IDE
- `.vscode/settings.json` points C/C++ IntelliSense at PlatformIO

Generated PlatformIO VS Code files stay ignored.

## Project Layout

```text
include/
  AppConfig.h          application timings, task stack sizes, priorities
  AppLog.h             multi-level serial logger without Arduino String
  AppTasks.h           FreeRTOS task bootstrap
  DiagnosticsLogger.h  Serial Monitor diagnostics API
  DisplayRenderer.h    OLED rendering API
  HardwareProfile.h    active board profile: pins, labels, display rotation
  MemoryDiagnostics.h  internal heap, PSRAM, and fragmentation telemetry
src/
  AppLog.cpp
  AppTasks.cpp         task creation and task loops
  DiagnosticsLogger.cpp
  DisplayRenderer.cpp
  MemoryDiagnostics.cpp
  main.cpp             Arduino setup/loop entrypoint
lib/
  U8g2/                local vendored OLED library
scripts/
  check_repo.py        repository policy regression checks
  checks.sh            check_repo + git diff --check + pio run
  install_hooks.sh     configures .githooks as Git hooks path
.githooks/
  pre-commit           runs scripts/checks.sh before commit
.vscode/
  extensions.json      recommends PlatformIO IDE
  settings.json        PlatformIO C/C++ IntelliSense provider
```

## FreeRTOS Tasks

| Task | Core | Priority | Period | Responsibility |
| --- | ---: | ---: | ---: | --- |
| `oled-render` | 1 | 2 | 500 ms | Render boot and base status screens |
| `serial-diag` | 0 | 1 | 5000 ms | Print periodic heartbeat diagnostics |

The Arduino `loop()` is intentionally idle and only calls `vTaskDelay()`.

The diagnostics task logs:

- internal heap free bytes
- internal heap largest free block, useful for fragmentation tracking
- PSRAM availability and free/largest block when PSRAM is present and enabled
- task stack high-water marks for the display and diagnostics tasks

## Memory And PSRAM Policy

The base firmware keeps runtime memory behavior explicit:

- Application code must not use Arduino `String`.
- Application code must not use generic heap APIs such as `new`, `delete`,
  `malloc`, `calloc`, `realloc`, or `free`.
- Periodic code must not use blocking `delay()`; use FreeRTOS delays.
- Large buffers in future module branches should be allocated once at startup
  with ESP-IDF capability-aware APIs such as `heap_caps_malloc()`.
- Use `MALLOC_CAP_SPIRAM` only for large, long-lived, non-DMA data on boards
  that actually have PSRAM.
- Keep DMA buffers, ISR state, task stacks, and latency-sensitive state in
  internal RAM.
- Monitor largest internal free block as well as total free heap, because
  fragmentation can break a future allocation even when total free bytes look
  acceptable.

The current `esp32dev` profile does not enable PSRAM globally. For a concrete
PSRAM board, add a board-specific environment in that module branch and enable
the PlatformIO Arduino ESP32 PSRAM flags there, for example:

```ini
[env:esp32dev-psram]
extends = env:esp32dev
build_flags =
    ${env.build_flags}
    -D BOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
```

Use the cache workaround flag only for targets that need it; keep PSRAM enablement
board-specific, not global on `main`.

## Local Libraries

U8g2 is stored in `lib/`, so the project builds from a local copy instead of
downloading the library into `.pio/libdeps`. Keep dependencies vendored in
`lib/` on `main`; module branches that add dependencies should vendor and
document them in the same change.

To refresh U8g2 manually, temporarily add it back to `lib_deps`, run `pio run`,
then copy the resolved package from `.pio/libdeps/esp32dev/` into `lib/U8g2`.

Module branches may add their own local libraries when needed.

## Engineering References

This baseline follows these upstream practices:

- PlatformIO project configuration supports shared and named `[env]` sections in
  `platformio.ini`.
- PlatformIO `build_flags` apply broadly, while `build_src_flags` are scoped to
  project source files.
- PlatformIO uses the project `lib/` directory for private/local libraries, and
  that directory has priority for Library Dependency Finder.
- ESP-IDF exposes memory capability APIs so firmware can choose internal RAM,
  DMA-capable RAM, or PSRAM deliberately.
- ESP-IDF recommends measuring largest free block alongside free heap to catch
  fragmentation early.
- ESP-IDF external RAM documentation treats PSRAM as useful for large buffers but
  calls out restrictions: cache-disabled periods, large-cache effects, and task
  stacks staying in internal memory by default.
- ESP-IDF FreeRTOS on ESP32-family targets supports core-pinned task creation for
  dual-core parts such as ESP32 and ESP32-S3.
- ESP-IDF logging uses tagged severity levels from error through verbose, with
  compile-time and runtime filtering concepts. `AppLog` mirrors that model for
  this Arduino baseline without introducing Arduino `String`.

References:

- <https://docs.platformio.org/en/latest/projectconf/index.html>
- <https://docs.platformio.org/en/latest/projectconf/sections/env/index.html>
- <https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_flags.html>
- <https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_src_flags.html>
- <https://docs.platformio.org/en/latest/projectconf/sections/platformio/options/directory/lib_dir.html>
- <https://docs.platformio.org/en/latest/platforms/espressif32.html#external-ram-psram>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/mem_alloc.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/heap_debug.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/external-ram.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/performance/ram-usage.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/performance/speed.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/performance/size.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html>
- <https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/log.html>

## OLED Screens

At boot, the OLED shows the repository baseline name, OLED pins, and Serial
Monitor speed.

After the boot screen, the OLED shows:

- base firmware ready status
- uptime
- OLED I2C pins
- reminder that module variants live on branches

All OLED screens use only the first five text rows to avoid the damaged bottom
line on this board.

## Branch Workflow

Use `main` as the starting point for a new hardware configuration:

```sh
git switch main
git switch -c module/<name>
```

Keep module-specific code, libraries, wiring notes, and troubleshooting in that
module branch. When replacing an internal API or config shape inside a branch,
migrate its current callers and remove the old path in the same change.

## Coding Rules

Project-specific engineering rules live in `AGENTS.md`. Key points:

- no Arduino `String`
- no generic heap allocation in application code
- no blocking `delay()` in task code
- no direct module-level `Serial.print*`; use `AppLog`
- board-specific values belong in `HardwareProfile.h`
- periodic work belongs in FreeRTOS tasks
- PSRAM use must be explicit and board-specific
- run `scripts/checks.sh` before handoff

## Troubleshooting

OLED does not display:

- This project starts with `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`.
- If your board uses a SH1106 or another controller, switch the U8g2 constructor in `src/DisplayRenderer.cpp` to the matching `U8G2_SH1106_128X64_*_HW_I2C` variant.
- Confirm OLED pins `SDA=GPIO5` and `SCL=GPIO4` for your exact board revision.
- This board variant renders correctly with `U8G2_R2`, which rotates the OLED by 180 degrees. Use `U8G2_R0` if your display is mounted the other way.
