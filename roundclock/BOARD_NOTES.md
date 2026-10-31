# Waveshare ESP32-S3-Touch-AMOLED-1.43: Project Notes

Context for picking this project back up later.

## What the board is

Sold on Amazon as "ESP32-S3 1.43inch AMOLED Display Development Board, 466×466" (brand: waveshare, sold by Ethink, ~$35.51). It shipped with no documentation. It is the **Waveshare ESP32-S3-Touch-AMOLED-1.43**.

- Official docs: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.43
- Product page: https://www.waveshare.com/product/esp32-s3-touch-amoled-1.43.htm
- Variants: the plain `-1.43` has no case, and `-1.43-B` has a metal case. **The `-1.43C` is a different board**, so don't use its docs or pins.
- Antenna: ceramic by default. There is an IPEX connector, but it needs a resistor moved to use it.

## Core specs

| Item | Value |
|---|---|
| MCU | ESP32-S3R8 (dual-core LX7, 240 MHz) |
| Flash | 16 MB |
| PSRAM | 8 MB, octal (OPI). This is the "R8". The "24 MB" in the listing is 16 + 8. |
| Wireless | 2.4 GHz Wi-Fi 802.11 b/g/n, BLE 5 |
| USB | USB-C, native ESP32-S3 USB (USB-CDC serial and JTAG) |
| Display | 1.43" round AMOLED, 466×466, 16.7M colors, 350 cd/m², 60000:1 contrast |
| Display driver IC | CO5300 (listing says "SH8601/C05300"). The QSPI interface works with `Arduino_CO5300`. |
| Touch | Capacitive, FT3168 (I2C) |
| RTC | PCF85063 |
| IMU | QMI8658 (6-axis) |
| Storage | TF / microSD slot |
| Other | Gold pin headers on the side ("multiple interfaces"), plus BOOT and RESET buttons |

## Pin map (from the Waveshare docs)

### AMOLED (QSPI)
| Signal | GPIO |
|---|---|
| CS | 9 |
| SCLK | 10 |
| D0 | 11 |
| D1 | 12 |
| D2 | 13 |
| D3 | 14 |
| RESET | 21 |
| **Power enable (AMOLED_EN)** | **42**, which must be driven HIGH or the screen stays dark |

### Shared I2C bus (touch, IMU, RTC)
| Signal | GPIO |
|---|---|
| SDA | 47 |
| SCL | 48 |
| IMU interrupt | 8 |
| RTC interrupt | 15 |

- The docs list no touch interrupt or reset pin. Find them before doing interrupt-driven touch.
- **I2C addresses, confirmed by scan (2026-10-28):** FT3168 touch `0x38`, PCF85063 RTC `0x51`, QMI8658 IMU `0x6B`. The scan also gets an ACK at `0x7E`, which is a reserved 7-bit address. It is probably a quirk or false positive and is harmless.

### Micro SD / TF card (SPI; slot is on the back of the PCB)
| Signal | GPIO |
|---|---|
| SD_CS | 38 |
| SD_MOSI | 39 |
| SD_MISO | 40 |
| SD_CLK | 41 |

**Warning:** GPIO38–41 are also brought out on the right-hand header, so they are shared with the SD card. Don't use them for anything else while a card is in use.

### Expansion headers (2×7, 2.54 mm; from the listing's "Interface Definition" image)

Left header, read as each row's (left label | right label):
| | |
|---|---|
| GPIO0 | GPIO1 |
| GPIO7 | GPIO2 |
| GPIO17 | GPIO3 |
| GPIO18 * | GPIO5 |
| GND | GPIO6 |
| 5V | USB_N (GPIO19) |
| BAT | USB_P (GPIO20) |

Right header:
| | |
|---|---|
| GPIO45 | 3V3 |
| GPIO46 * | TXD |
| GPIO41 (SD_CLK) | RXD |
| GPIO40 (SD_MISO) | GND |
| GPIO39 (SD_MOSI) | SDA |
| GPIO38 (SD_CS) | SCL |
| GPIO18 * | 3V3 |

\* Discrepancies to check with a multimeter or the schematic:
- The listing labels GPIO18 on **both** headers, but the left header's silkscreen seems to read **IO16** in that spot. It's probably GPIO16 on the left.
- The right header's second-row silkscreen may read IO44 rather than GPIO46.
- TXD/RXD on the right header are probably UART0 (GPIO43/44). The header's SDA/SCL are probably the shared bus on GPIO47/48. Neither is confirmed.

Pin cautions (ESP32-S3 general):
- GPIO0 is the BOOT strapping pin, and GPIO45/46 are also strapping pins. Don't pull them hard at reset.
- GPIO19/20 are the native USB D-/D+. Using them as GPIO kills USB serial and upload.
- GPIO35–37 are taken by the octal PSRAM and aren't exposed, which is good.

## On-board connectors and parts (from the "What's On Board" image)
1. ESP32-S3R8
2. 16 MB flash
3. QMI8658 6-axis IMU (3-axis gyro and 3-axis accelerometer). The X/Y/Z axis arrows are printed on the PCB next to it.
4. PCF85063 RTC
5. **3.7 V Li-ion/LiPo battery header** ("BAT", 2-pin, top). The board has charging: a CHG LED lights while charging.
6. TF card slot (back of the PCB)
7. **RTC backup battery header** (2-pin, left)
8. **I2C 4-pin connector** (bottom left; silkscreen roughly GND/3V3/SDA/SCL. Check the order.)
9. Charge indicator LED ("CHG")
10. USB Type-C
11. Power indicator LED ("PWR")
12. **UART 4-pin connector** (bottom right; silkscreen roughly GND/3V3/TXD/RXD)
13. BOOT button
14. RESET button
15. IPEX1 antenna connector (needs a resistor moved)
16. Onboard ceramic antenna

The small connectors look like JST-SH/GH-style 1.0/1.25 mm pitch. Measure before ordering cables.

## Working PlatformIO setup (confirmed: builds, uploads and displays)

- The project folder is `Desktop\claude\amoled143`. Open **this folder** in VS Code, not its parent.
- Platform: **pioarduino** (Arduino-ESP32 core 3.x), using `platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip`. The official `espressif32` platform is stuck on core 2.x, which newer Arduino_GFX doesn't support.
- Board settings: `esp32-s3-devkitc-1` as the base, with these overrides:
  - `board_build.arduino.memory_type = qio_opi`
  - flash size 16MB, using `default_16MB.csv`
  - `-DBOARD_HAS_PSRAM -DARDUINO_USB_MODE=1 -DARDUINO_USB_CDC_ON_BOOT=1`
- Library: `moononournation/GFX Library for Arduino@^1.6.0`
- Display init in code:
  ```cpp
  Arduino_DataBus *bus = new Arduino_ESP32QSPI(9, 10, 11, 12, 13, 14);
  Arduino_GFX *gfx = new Arduino_CO5300(bus, 21, 0, 466, 466, 6, 0, 0, 0); // col offset 6
  // pinMode(42, OUTPUT); digitalWrite(42, HIGH); before gfx->begin()
  ```
- Test sketches: the first one cycled the screen through red, green, blue and white with "Hello!". It was later replaced by an LVGL and Nixie One font test, now kept in `extras/lvgl_font_test.cpp`. `src/` now holds the clock project (see `CLOCK_PLAN.md`).
- **Offsets verified by eye (2026-10-28):** column offset 6 and row offset 0 give rings that look concentric with the glass. If the image ever looks shifted, the Phase 1 calibration firmware (`CALIBRATE` in `main.cpp`) can change both live over serial.

## Fixes for setup problems I've hit (Windows PC "inky")

1. **PlatformIO icon missing:** the PlatformIO IDE extension wasn't installed. Install it from the VS Code Extensions view.
2. **`IncompatiblePlatform ... not compatible with PlatformIO Core v6.2.0-post.1 ... depends on >=6.2.0`:** a dev build of Core was installed, and `-post.1` counts as older than `6.2.0`. Fix: uncheck "Use Development PIO Core" in VS Code settings, then run `pio upgrade`.
3. **`FileNotFoundError` while unpacking `esp32-core-3.x-libs.tar.xz` (very long `esp_matter/connectedhomeip/...` path):** Windows' 260-character path limit. Fix: set `HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnabled = 1` (DWORD) in an admin PowerShell, restart, clear `~\.platformio\.cache\tmp` and the partial `framework-arduinoespressif32*` packages, then rebuild. **Done; this fixed it.**

## Upload and flash tips

- If upload fails or the COM port doesn't show up: hold BOOT, tap RESET, release BOOT, then upload. Press RESET afterwards.
- Serial uses native USB CDC, so the COM port may disappear and come back on reset.

## Ideas and next steps

- **Clock project:** see `CLOCK_PLAN.md` (minimal hour + minute-arc clock with a hotspot-based time setter).
- Touch: FT3168 on I2C 47/48. Arduino_GFX doesn't handle touch, so use a FocalTech FT3x68/FT6x36-style driver or plain I2C reads (Waveshare's examples use their own driver).
- LVGL: Waveshare's marketing pushes it (LVGL with SquareLine Studio). Allocate the draw buffers in PSRAM.
- The marketing images show a smartwatch-style face (time, date, weather, steps, distance in a colored ring). That's a natural project using the RTC, IMU step counting and Wi-Fi.
- Waveshare's own Arduino and ESP-IDF demo code is linked from the docs page under "Working with Arduino" and "Resources". It's worth pulling for touch, IMU and RTC pin and driver details.
- AMOLED note: black pixels are off, so dark backgrounds save power and avoid burn-in. Avoid leaving static bright images on screen for long.

## Phase 1 hardware results (2026-10-28)

- **Visible area:** the outermost 1 px ring of the 466 px circle (radius 232 to 233 from the true centre 233.0) is hidden under the glass edge. The ring 2 px in is visible. Keep anything important at least 2 px in from the edge; the clock uses `EDGE_MARGIN = 4`.
- **Centre:** the panel is 466 px, an even number, so the geometric centre is at 233.0, *between* pixels 232 and 233. Draw with float coordinates around 233.0 for symmetric rings.
- **Brightness:** `setBrightness()` (DCS 0x51) works. 255, 128 and 32 are clearly different. The panel's init value is 208 (0xD0).
- **Touch (FT3168):** coordinates match the display (x to the right, y down). A press in the middle reads about (241, 244), and the top-left area about (106, 55).
  - **Gotcha:** the chip NAKs one I2C read about every 13.7 s, like clockwork, whether or not it's being touched. The core then logs `i2cWriteReadNonStop ... ESP_ERR_INVALID_STATE` (that's how the new IDF I2C driver reports a NAK). At a 50 ms poll that's about 4 of 1200 reads a minute.
    - Writing `0x86 = 0` and `0xA5 = 0` (FocalTech "stay active") at init didn't change it. The writes are still in `touch::begin()` and are harmless.
    - It's benign as long as a failed read is treated as "no change" (`input.cpp` / `calibrate.cpp` do this).
    - To hide the log line in release builds, use `-DCORE_DEBUG_LEVEL=0`.
- **RTC (PCF85063):**
  - Keeps counting across the RESET button.
  - After unplugging USB (there is no backup cell), it comes back as `2000-01-01 00:00:xx` with the OS flag set. That is the "time invalid, go to set-time mode" case.

## Compile-checking from a cloud agent (no USB, no `pio` on Kyle's remote shell)

A cloud agent can't flash the board, but it can confirm the code compiles before handing off to Kyle. What worked on 2026-10-28:

1. **Get the code:** stage the project's text files into the container: `platformio.ini`, `include/*`, `src/**`. Skip `.pio/`.
2. **Install PlatformIO:** `pip install platformio --break-system-packages`.
3. **TLS through the agent proxy:**
   - Export `REQUESTS_CA_BUNDLE` and `SSL_CERT_FILE` pointing at `/root/.ccr/ca-bundle.crt`.
   - PlatformIO's penv and pioarduino also use their own certifi bundles: pioarduino points `SSL_CERT_FILE` at the penv's certifi. Append **the whole** `/root/.ccr/ca-bundle.crt` to every `certifi/cacert.pem`, and do it again after the first `pio run` creates `~/.platformio/penv` (it doesn't exist before). Appending only `agent-proxy-ca.crt` wasn't enough for `uv` (UnknownIssuer), and a "skip if already there" check keyed on the bundle's first line skips every file, because that line is a standard CA.
   - The penv's `uv` still failed on pypi.org until the `pio run` shell also had `UV_NATIVE_TLS=1`. Symptom: `Failed to install Python dependencies into penv` with no detail; the hidden error is `invalid peer certificate: UnknownIssuer`.
4. **The PlatformIO registry is blocked:** `api.registry.platformio.org` returns 403 through the proxy. GitHub downloads work, so:
   - **`tool-scons`:** build it by hand.
     1. `pip download scons` (4.11.1).
     2. Unzip the wheel into `~/.platformio/packages/tool-scons/`.
     3. Add a `scons.py` that puts its own folder on `sys.path` and calls `SCons.Script.Main.main()`.
     4. Add a `package.json` and a `.piopm` with `{"type":"tool","name":"tool-scons","version":"4.41101.0","spec":{"owner":"platformio","id":null,"name":"tool-scons","requirements":"~4.41101.0","uri":null}}`.
   - **Libraries:** in the *container copy* of `platformio.ini` only, swap `lib_deps` to the Git URLs for the same versions Kyle has: `https://github.com/moononournation/Arduino_GFX.git#v1.6.9` and `https://github.com/lvgl/lvgl.git#v9.2.2`. Kyle's `.pio/libdeps` has GFX 1.6.9 and LVGL 9.2.2. Never commit that change back.
   - Turn telemetry off: `pio settings set enable_telemetry No`. `collector.platformio.org` is blocked as well.
5. **Build:** run `pio run`. The first build downloads about 1 GB of toolchain and takes about 3 minutes; incremental builds take about 15 s.
6. **Reference result:** Arduino core 3.3.12 (IDF 5.5), Phase 1 firmware: RAM 7.7 %, flash 9.7 %, no warnings from project code. Phase 2: RAM 7.8 %, flash 10.0 %. Phase 3: RAM 7.9 %, flash 10.6 %. Phase 4 (Wi-Fi, WebServer, DNSServer): RAM 15.4 %, flash 22.6 %. Phase 6: unchanged. Changing `lv_conf.h` rebuilds all of LVGL (~2 min): run `pio run` in the background if the shell has a 2-minute limit.
   - Re-verified 2026-10-28 (fresh container): the first `pio run` creates the penv and then fails downloading `esp32-core-3.3.12.tar.xz` from GitHub with `CERTIFICATE_VERIFY_FAILED`. Re-append the CA bundle to every `certifi/cacert.pem` (`find / -name cacert.pem -path '*certifi*'`) and rerun: then it builds (~4 min).
7. **Moving files:** stage a tarball (`tar czf .pio/snapshot.tgz platformio.ini include src tools NixieOne-Regular.ttf` on Kyle's side) instead of many files. To send results back, copy a tarball to `/mnt/user-data/outputs/`, `device_commit_files` it into `.pio/`, extract it to `$HOME` on the remote shell, then `cat new > old` for each file. Plain `tar x` over existing files fails there, because the remote shell isn't allowed to delete (unlink) files. Check the originals' md5 against the snapshot first so you don't overwrite an edit Kyle made meanwhile.

### Previewing the face on a PC (`tools/host_sim`)

Also `tools/host_sim/dim_test.cpp` tests the night-dimming schedule and fade maths (`g++ -std=c++17 -I../../src -I../../include dim_test.cpp -o dim_test && ./dim_test`; must print "all dim tests passed"), and `tools/host_sim/portal_test.cpp` tests the portal's JSON parsing and time-zone choice (`g++ -std=c++17 -I../../src portal_test.cpp -o portal_test && ./portal_test`); it must print "all tests passed".

`tools/host_sim/` builds `face.cpp` + `ring_render.cpp` against real LVGL 9.2.2 on Linux, with a small `Arduino.h` stand-in, and dumps 466×466 RGB565 frames (`make LVGL=<lvgl path>`, `./sim`, `python3 to_png.py a.raw a.png`; details in `sim.cpp`). In the container, LVGL is at `.pio/libdeps/amoled143/lvgl` after a `pio run`. Add `-fsanitize=address` to catch memory bugs: that's how the tiny_ttf GPOS bug below was found.

## LVGL 9.2.2 tiny_ttf gotcha (found 2026-10-28)

`stbtt_KernTableCheck()` in `src/libs/tiny_ttf/stb_truetype_htcw.h` (called by every `lv_tiny_ttf_create_*`) reads the GPOS lookup list without adding the GPOS table offset. On a font that has GPOS (Nixie One does), it reads a garbage lookup count (47338) and walks up to ~160 KB past the end of the font data. Workaround: embed fonts through `tools/embed_ttf.py`, which subsets to digits + space and drops GPOS/GSUB/GDEF/kern.

The library source in `.pio/libdeps/amoled143/` inside the container is handy for checking APIs. For example, `Arduino_TFT` keeps `COL_OFFSET1`, `ROW_OFFSET1`, `_xStart` and `_currentX` as protected members, which is what makes the `ClockCO5300::setOffsets()` subclass possible.

## Display performance (measured 2026-10-28, Phase 2 firmware)

- `ring::render` (float SDF renderer into a PSRAM canvas): ~3.5 µs/pixel inside the ring zone, about 30× slower than a PC. A full ring redraw takes 156 ms; a 1-second band update takes ≤ 2 ms.
- Hour-clear animation, with each frame rendered and pushed via `lv_refr_now()`: up to 21 ms per frame for a ~1100 px area, which is ~39 fps.
- First draw of a new 260 px Nixie One numeral (tiny_ttf rasterising): ~80 ms.
- Free memory while running: 314 KB internal, 7437 KB PSRAM.

## Timekeeping notes (Phase 3, 2026-10-28)

- `adjtime()` exists in this core's newlib (`libnewlib.a: time.c`), so the system clock can be slewed rather than stepped. The IDF slews at about 1/64 of real time.
- `time_t` is 64-bit on IDF 5, and `gmtime_r`/`localtime_r` with a POSIX TZ string behave like glibc's for the US rules.
- **PCF85063 write timing confirmed:** after the STOP bit is released, the first seconds tick comes ~0.5 s later. Writing second S at system time S + 0.5 s left the RTC +3 ms from the system clock (measured on the board).
- NVS: `Preferences` namespace `clock`, keys `tz` (POSIX string) and `set_at` (unix seconds of the last set).

## Logging (Phase 6, 2026-10-29)

- In this core, `CORE_DEBUG_LEVEL` isn't set by PlatformIO, so Arduino's `log_x` level comes from the prebuilt sdkconfig: `CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL = 1` (errors only). That's why the FT3168 NAK showed up as an `E` line. The clock leaves this at the default; add `-DCORE_DEBUG_LEVEL=0` to `build_flags` to silence it entirely.
- The project's own chatty output is behind `cfg::DEBUG_LOG` in `clock_config.h`.
- In scons builds, `touch` doesn't force a rebuild (content signatures). Delete `.pio/build/amoled143/src/*.o` to recompile the project files and see their warnings.

