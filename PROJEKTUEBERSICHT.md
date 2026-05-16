# Projektübersicht: ESP32C3_TRINKET

Stand: 16.05.2026

## Zweck

Dieses Repository enthält ein importiertes ESP32-C3-Projekt. Das Unterprojekt `miniapp` zeigt Uhrzeit, Datum, Stadt, Wetterdaten, Luftqualitätswerte und kleine Symbolgrafiken auf einem 128 x 128 Pixel großen TFT-Display.

Diese Datei ist als Einstiegskontext für neue Codex-Tasks gedacht. Vor Änderungen zuerst diese Datei, `miniapp/platformio.ini` und die relevanten Stellen in `miniapp/src/main.cpp` lesen.

## Projektstruktur

- `miniapp/`: PlatformIO-Projekt.
- `miniapp/platformio.ini`: Board-, Flash-, Display- und Bibliothekskonfiguration.
- `miniapp/src/main.cpp`: Hauptprogramm, aus `miniapp.ino` portiert.
- `miniapp/src/weathernum.cpp`: Auswahl der Wettersymbole.
- `miniapp/src/TKR.c` und `miniapp/src/TKR_A.c`: importierte LVGL-Bilddaten.
- `miniapp/include/`: Projekt-Header, Bilddaten und Schriftarten.
- `lib/`: lokale Arduino-Bibliotheken, darunter eine angepasste lokale Kopie von `TFT_eSPI`.
- `miniapp/miniapp.ino`: ursprüngliche Arduino-Datei, nur noch als Referenz verwenden.

## Hardware

- Board: ESP32C3 Dev Module.
- Aktueller Anschluss: `COM8`.
- Buttons: IO8, IO9 und IO10 sind mit Tastern belegt.
- USB CDC On Boot: deaktiviert.
- Flash Mode: QIO.
- Flash Size: 4 MB.
- Partition Scheme: Huge APP, 3 MB App, kein OTA, 1 MB SPIFFS.
- PlatformIO-Boardprofil: `esp32-c3-devkitm-1`.

## PlatformIO

Das Projekt baut mit PlatformIO und Arduino-Framework. Wichtige Konfiguration in `miniapp/platformio.ini`:

- `platform = espressif32`
- `board = esp32-c3-devkitm-1`
- `framework = arduino`
- `lib_extra_dirs = ../lib`
- `lib_ignore = HttpClient`
- `upload_port = COM8`
- `monitor_port = COM8`
- `monitor_speed = 115200`
- `board_build.flash_mode = qio`
- `board_build.flash_size = 4MB`
- `board_upload.flash_size = 4MB`
- `board_build.partitions = huge_app.csv`

Build:

```powershell
pio run
```

Upload:

```powershell
pio run --target upload
```

Serieller Monitor:

```powershell
pio device monitor --port COM8 --baud 115200
```

Auf diesem Windows-System liegt `pio.exe` typischerweise unter:

```powershell
$env:APPDATA\Python\Python313\Scripts\pio.exe
```

## Display

Das Display wird über `TFT_eSPI` als ST7735 angesteuert. Die TFT-Konfiguration steht nicht in einer separaten `User_Setup.h`, sondern in `platformio.ini` per Build-Flags:

- Treiber: `ST7735_DRIVER`
- Farbfolge: `TFT_BGR`
- Breite: 128 Pixel
- Höhe: 128 Pixel
- ST7735-Variante: `ST7735_GREENTAB3`
- Invertierung: `TFT_INVERSION_ON`
- MOSI: GPIO 4
- SCLK: GPIO 3
- CS: GPIO 2
- DC: GPIO 0
- RST: GPIO 5
- SPI-Frequenz: 27 MHz

Im Code wird das Display in `setup()` initialisiert:

- `tft.begin()`
- `tft.invertDisplay(0)`
- `tft.setRotation(2)`

Der zuletzt getestete Zustand zeigt eine vollständige, brauchbare Anzeige. Frühere Fehlerbilder waren weißer Bildschirm, Reboot-Schleife und vertikale Farbstreifen.

## Lokaler TFT_eSPI-Fix

Die lokale Bibliothek `lib/TFT_eSPI` wurde bewusst angepasst. Diese Änderungen nicht durch eine frische Library-Version überschreiben, ohne den Displaytest erneut zu machen.

Geänderte Dateien:

- `lib/TFT_eSPI/Processors/TFT_eSPI_ESP32_C3.h`
- `lib/TFT_eSPI/Processors/TFT_eSPI_ESP32_C3.c`

Grund: Der ursprüngliche Low-Level-SPI-Registerpfad von `TFT_eSPI` löste auf dem verwendeten Arduino-ESP32-Core auf dem ESP32-C3 Store- und Load-Access-Faults aus. Die Abstürze traten zuerst bei `tft.begin()` und danach bei `fillScreen()` auf.

Die C3-Anpassung macht Folgendes:

- Direkte SPI-Registerumschaltung für Write/Read-Modus deaktiviert.
- Direkten `_spi_cmd`-Busy-Check für C3 deaktiviert.
- `tft_Write_*`, `pushBlock()`, `pushPixels()` und `pushSwapBytePixels()` für C3 auf stabile Arduino-SPI-Schreibpfade umgestellt.
- `pushPixels()` und `pushSwapBytePixels()` geben Pixeldaten bytegenau aus. Das war wichtig, um vertikale Farbstreifen in Sprites und JPG-Ausgaben zu vermeiden.

## WLAN

Das Testnetz ist bewusst direkt im Code hinterlegt. Das ist für dieses Testprojekt akzeptiert.

- SSID: `Agathas-Netz-16`
- Passwort: `1234567890123050363`

Die Werte stehen in `miniapp/src/main.cpp` nahe Dateianfang als `ssid` und `password`. Es gibt aktuell keine produktive Geheimnisverwaltung, keine EEPROM-basierte WLAN-Konfiguration und keine aktive SmartConfig-Funktion.

`get_wifi()` startet `WiFi.begin(ssid, password)` und bricht nach 30 Sekunden ohne Verbindung ab. Das verhindert eine endlose Blockade beim Start, wenn das Testnetz nicht verfügbar ist.

## Akku und Deep Sleep

Das Board läuft nach dem Abziehen von USB über den Akku weiter. Deshalb gibt es eine Long-Press-Funktion auf IO8:

- IO8 wird als `INPUT_PULLUP` verwendet.
- Gedrückt bedeutet `LOW`.
- Langes Drücken für 2,5 Sekunden startet den Deep Sleep.
- Vor dem Deep Sleep wird der Bildschirm gelöscht und zeigt `Schlafmodus`, `Taste loslassen` und `IO8 weckt auf`.
- Der Code wartet auf das Loslassen von IO8, damit das Board nicht sofort wieder aufwacht.
- Danach wird der ST7735 mit `DISPOFF` und `SLPIN` in den Display-Sleep-Modus geschickt. Zusätzlich wird `LCD_RST` auf IO5 auf `LOW` gesetzt und per GPIO-Hold während Deep Sleep gehalten. Danach wird WLAN deaktiviert, der serielle Puffer wird geleert und IO8 wird als Deep-Sleep-Wakeup-Quelle mit `ESP_GPIO_WAKEUP_GPIO_LOW` aktiviert.
- Neustart ist über erneutes Drücken von IO8 oder über Reset möglich.

Die Implementierung steht in `miniapp/src/main.cpp` in den Funktionen `configureDeepSleepButton()`, `handleDeepSleepButton()`, `enterDeepSleep()`, `prepareDisplayForDeepSleep()` und `waitForDeepSleepButtonRelease()`.

Wichtig: Im Display-Schaltplan hängt `LEDA` über `R11 = 5R1` fest an `VCC3.3`; `LEDK` liegt fest an GND. Die Hintergrundbeleuchtung hat damit keinen Software-Schaltpin. `LCD_RST` kann nur den Displaycontroller zurücksetzen, nicht die Backlight-LED abschalten. Für minimalen Ruhestrom wäre hardwareseitig ein schaltbarer Backlight-Zweig nötig, zum Beispiel R11 auftrennen und die LED-Anode über einen P-MOSFET oder Load-Switch schalten. Alternativ könnte ein Low-Side-Schalter in den LEDK-Zweig, falls dieser Leiterzug besser zugänglich ist.

## Externe Dienste

Die App nutzt:

- `ip-api.com` zur Stadtermittlung über die öffentliche IP-Adresse.
- OpenWeatherMap für Wetterdaten und Zeitzoneninformationen.
- NTP über `time.nist.gov`.

Der OpenWeatherMap-Schlüssel ist im Quellcode hinterlegt. Für produktive Nutzung sollte er separat verwaltet werden.

## LVGL und Startanimation

LVGL ist weiterhin als Bibliothek eingebunden, aber die importierte GIF-Startanimation `TKR_A` ist im Bootpfad deaktiviert. Sie verursachte auf dem ESP32-C3 einen Laufzeitabsturz im LVGL-Bilddecoder.

Die Hauptanzeige nutzt anschließend `TFT_eSPI` direkt und funktioniert unabhängig von dieser Startanimation. Wenn LVGL wieder aktiviert werden soll, zuerst Speicherbedarf, Decoderpfad und Stacktraces prüfen.

## Lokalisierung und Code-Stil

Projektvorgaben aus `AGENTS.md`:

- User-facing Strings auf Deutsch lokalisieren.
- Deutsche Umlaute verwenden.
- Datumsformate im EU-Format `DD.MM.YYYY`.
- Variablen- und Methodennamen nicht unnötig abkürzen.

Die chinesischen Kommentare des importierten Projekts wurden in den eigenen Projektdateien fachlich auf Deutsch neu formuliert. Third-Party-Code unter `lib/` wurde nur dort angepasst, wo es für die ESP32-C3-Hardware notwendig war.

## Bekannte Diagnosehinweise

Wenn das Board wieder rebootet:

1. Seriellen Monitor mit `115200` Baud starten.
2. `MEPC`-, `RA`- und Stack-Adressen aus der Guru-Meditation notieren.
3. Gegen das aktuelle ELF dekodieren:

```powershell
$env:USERPROFILE\.platformio\packages\toolchain-riscv32-esp\bin\riscv32-esp-elf-addr2line.exe -pfiaC -e .pio\build\esp32c3dev\firmware.elf <adresse1> <adresse2>
```

Wenn das Display Streifen zeigt:

- Zuerst `pushPixels()` und `pushSwapBytePixels()` in `lib/TFT_eSPI/Processors/TFT_eSPI_ESP32_C3.c` prüfen.
- Pixeldaten nicht als falschen Worttyp reinterpretieren; einige Aufrufer liefern Roh-Bytes.
- `TJpgDec.setSwapBytes(true)` ist aktuell gesetzt und Teil des getesteten Zustands.

Wenn der Bildschirm weiß bleibt:

- Prüfen, ob `tft.begin()` erreicht wird.
- Danach ST7735-Variante, Farbfolge, Invertierung, Rotation und Pinbelegung prüfen.
- Der zuletzt funktionierende Zustand nutzt `ST7735_GREENTAB3`, `TFT_BGR`, `tft.invertDisplay(0)` und Rotation `2`.
