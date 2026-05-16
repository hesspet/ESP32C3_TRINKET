# ESP32C3_TRINKET

ESP32C3_TRINKET is a PlatformIO/Arduino firmware project for an ESP32-C3 board with a 128 x 128 pixel ST7735 TFT display. The `miniapp` firmware shows time, date, city, weather data, air-quality related values, and small bitmap icons on the display.

Project context is maintained in [PROJEKTUEBERSICHT.md](PROJEKTUEBERSICHT.md). Last documented project status: 16.05.2026.

## Fork Status

This repository is a fork/import of the original `ESP32C3_1.44inch` project. The fork keeps the original display-oriented mini application as a reference point, but the active code has been ported and stabilized for the currently used ESP32-C3 hardware and PlatformIO workflow.

The original Arduino sketch is still present as `miniapp/miniapp.ino`, but it is retained only as a reference. The maintained firmware entry point is `miniapp/src/main.cpp`.

## Main Changes Compared With the Original

- Ported the project into a PlatformIO layout under `miniapp/`.
- Configured the target as `esp32-c3-devkitm-1` with Arduino framework support.
- Moved the active application code from the imported Arduino sketch into `miniapp/src/main.cpp`.
- Centralized the ST7735/TFT_eSPI display setup in `miniapp/platformio.ini` build flags instead of relying on a separate `User_Setup.h`.
- Added and tested ESP32-C3 display settings for the 128 x 128 ST7735 panel:
  - `ST7735_DRIVER`
  - `TFT_BGR`
  - `ST7735_GREENTAB3`
  - `TFT_INVERSION_ON`
  - 27 MHz SPI
  - MOSI GPIO 4, SCLK GPIO 3, CS GPIO 2, DC GPIO 0, RST GPIO 5
- Added a local ESP32-C3-specific fix to `lib/TFT_eSPI` to avoid Store/Load Access Fault crashes in the low-level SPI path.
- Reworked C3 pixel writes in the local TFT_eSPI copy so sprite and JPEG output are written byte-accurately, avoiding vertical color stripes.
- Added a long-press deep-sleep path on IO8.
- Added display sleep preparation before deep sleep: clear screen, send ST7735 `DISPOFF` and `SLPIN`, hold `LCD_RST` low, stop Wi-Fi, flush serial output, and configure IO8 as the wake source.
- Disabled the imported LVGL GIF startup animation in the boot path because it caused a runtime crash in the LVGL image decoder on the ESP32-C3.
- Reworked project comments in the maintained project files into German.

## Hardware

- Board profile: `esp32-c3-devkitm-1`
- Current serial port: `COM8`
- USB CDC on boot: disabled
- Flash mode: QIO
- Flash size: 4 MB
- Partition scheme: Huge APP, 3 MB application, no OTA, 1 MB SPIFFS
- Buttons: IO8, IO9, and IO10
- Deep-sleep button: IO8, active low with internal pull-up

## Backlight Limitation

The TFT backlight cannot be switched off by firmware on the current hardware.

The display backlight is hard-wired in the schematic:

- `LEDA` is connected to `VCC3.3` through `R11 = 5R1`.
- `LEDK` is connected directly to GND.

Because there is no GPIO, transistor, MOSFET, or load switch in the LED supply path, the ESP32-C3 can only put the ST7735 display controller into sleep mode. It cannot cut power to the backlight LED.

`LCD_RST` on GPIO 5 can reset or hold the display controller, but it does not disconnect the backlight. During deep sleep the firmware therefore reduces what it can control in software, while the backlight remains a hardware-level current consumer.

To minimize sleep current further, the hardware would need a switchable backlight path. Practical options are:

- Open the `R11`/`LEDA` supply path and drive the LED anode through a P-MOSFET or load switch.
- Add a low-side switch in the `LEDK` path if that trace is easier to access.

## External Services

The firmware uses:

- `ip-api.com` to derive the current city from the public IP address.
- OpenWeatherMap for weather and timezone information.
- NTP via `time.nist.gov`.

The current test Wi-Fi credentials and OpenWeatherMap key are stored directly in `miniapp/src/main.cpp`. This is acceptable for the current test project, but should be changed before using the project outside a controlled local setup.

## Build

Run PlatformIO from the `miniapp` directory:

```powershell
pio run
```

On the documented Windows setup, `pio.exe` is typically available at:

```powershell
$env:APPDATA\Python\Python313\Scripts\pio.exe
```

## Upload

```powershell
pio run --target upload
```

## Serial Monitor

```powershell
pio device monitor --port COM8 --baud 115200
```

## Troubleshooting Notes

If the board reboots, capture the serial output at 115200 baud and decode `MEPC`, `RA`, and stack addresses against the current ELF.

If the display shows vertical color stripes, check the ESP32-C3 pixel write paths in:

- `lib/TFT_eSPI/Processors/TFT_eSPI_ESP32_C3.c`
- `lib/TFT_eSPI/Processors/TFT_eSPI_ESP32_C3.h`

If the display stays white, check the ST7735 variant, color order, inversion, rotation, and pin mapping. The last known working display setup uses `ST7735_GREENTAB3`, `TFT_BGR`, `tft.invertDisplay(0)`, and rotation `2`.
