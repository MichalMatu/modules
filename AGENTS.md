# Repository Engineering Guide

This repository is the shared ESP32-family PlatformIO/Arduino baseline and a
template for different combinations of ESP32-family boards, displays, sensors,
radios, and other modules. Keep `main` module-free and reusable for future
ESP32, ESP32-S3, and related board profiles.

## Architecture

- Put board-specific pins, labels, display rotation, and hardware assumptions in
  `include/HardwareProfile.h`.
- Keep timing, stack sizes, priorities, serial baud, and application constants in
  `include/AppConfig.h`.
- Keep `src/main.cpp` minimal: initialize platform services, initialize logging,
  start application tasks, and keep `loop()` idle.
- Add device features as modules with small interfaces in `include/` and
  implementations in `src/`.
- Keep long-running or periodic work in explicit FreeRTOS tasks. Do not block the
  display or diagnostics task with sensor I/O.

## C++ And Memory

- Use modern C++ suitable for embedded builds; this baseline builds with
  `-std=gnu++17`.
- Do not use Arduino `String`. Use fixed-size buffers, string literals, and
  `snprintf`.
- Avoid generic heap allocation in normal runtime paths. Do not use `new`,
  `delete`, `malloc`, `calloc`, `realloc`, or `free` from application code.
  Prefer `constexpr`, stack-local fixed buffers, and static storage where
  ownership is clear.
- If a module genuinely needs heap memory, use ESP-IDF capability-aware
  allocation explicitly, for example `heap_caps_malloc()` with the required
  capability flags. Do not hide internal RAM vs PSRAM decisions behind a generic
  fallback.
- Use PSRAM only for large, long-lived, non-DMA buffers on boards that actually
  have PSRAM and enable it in their PlatformIO environment. Keep task stacks,
  ISR data, DMA buffers, and latency-sensitive state in internal RAM.
- Pre-allocate buffers outside ISR paths. Do not allocate or free heap memory
  from interrupts.
- Prefer integer or fixed-point calculations for repeated sensor processing.
  Use `float` only when the sensor math or external API materially requires it.
- Track task stack high-water marks and internal heap largest-block values while
  tuning module branches.
- Keep log messages and display text concise. Do not build dynamic strings only
  to print them once.

## Logging And Diagnostics

- Use `AppLog` macros: `APP_LOGE`, `APP_LOGW`, `APP_LOGI`, `APP_LOGD`,
  `APP_LOGV`.
- Keep log tags short and stable, for example `startup`, `tasks`, `display`,
  `diag`, or the module name.
- Do not call `Serial.print*` directly from application modules unless a low-level
  transport implementation requires it. Route diagnostics through `AppLog`.
- Keep expensive logs at `APP_LOGD` or `APP_LOGV`; levels above
  `APP_LOG_LEVEL` are preprocessor-disabled and should not carry runtime cost.
- New modules should add startup logs and heartbeat-visible state when useful.
- Keep memory telemetry visible in diagnostics when a module adds buffers,
  wireless stacks, filesystem use, or parsing-heavy code.

## PlatformIO And Libraries

- Keep libraries local in `lib/` for reproducible builds in VS Code/PlatformIO.
- Do not add remote `lib_deps` on `main`. If a module needs a new dependency,
  vendor it into `lib/` and document the source and version in README.
- Keep `platformio.ini` declarative. Use build flags for project-wide compile
  switches such as log level and C++ standard.
- Use `build_src_flags` for strict flags that should apply only to project
  source files. Avoid applying project-only warning or C++ flags to vendored C
  libraries and framework code.
- Add PSRAM flags only in board-specific environments for hardware that includes
  PSRAM. For Arduino ESP32 in PlatformIO this is typically `BOARD_HAS_PSRAM`,
  with the ESP32 cache workaround flag when the target requires it.

## Branching

- `main` is a reusable baseline, not a feature branch.
- Do not add concrete module wiring, sensor logic, or board experiments directly
  on `main`.
- Put concrete sensor builds, experimental wiring, and board-specific variants on
  dedicated `module/<name>` branches.
- When the user names a board and modules they want to combine, first check
  whether the hardware details are sufficient: exact board variant, module
  breakout pin labels, voltage levels, required buses, available pins, power
  limits, bootstrapping pins, ADC needs, and whether Wi-Fi/BLE will be used.
- Ask concise follow-up questions only for uncertainties that can change wiring,
  power safety, pin selection, bus choice, or build configuration. If the
  remaining details can be inferred safely from the repo and common ESP32
  constraints, proceed with those assumptions and document them.
- Create or switch to a dedicated branch before implementing a new concrete
  module combination. Use clear names such as `module/air-quality-esp32-oled`,
  `module/gps-oled-18650`, or `module/<main-sensor-or-purpose>-<board>`.
- When replacing an internal API or config shape, migrate current callers and
  remove the old path in the same change. Do not keep redundant compatibility
  branches.

## Verification

Before handing off code changes, run:

```sh
scripts/checks.sh
```

For display or pin profile changes, also verify that README and
`HardwareProfile.h` describe the same hardware.

## Regression Checks And Hooks

- Keep repository policy checks in `scripts/check_repo.py`.
- Keep `scripts/checks.sh` as the single local verification entrypoint.
- Keep `.githooks/pre-commit` aligned with `scripts/checks.sh`.
- Install hooks in a clone with `scripts/install_hooks.sh`.
- Keep checks strict on avoidable memory risks: no Arduino `String`, no generic
  heap allocation, no blocking `delay()`, and no direct module-level
  `Serial.print*`.
