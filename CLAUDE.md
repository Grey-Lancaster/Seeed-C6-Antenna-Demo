# CLAUDE.md

Seeed-C6-Antenna-Demo: an Improv WiFi (credentials over USB serial) demo with antenna switching for an ESP32-C6 board (the repo name says Seeed), built from the owner's working ESP32 Improv example. Public repo with no README. Note that what is committed on `main` is an early generic-ESP32 test version; see Gotchas.

## Layout
- `platformio.ini` - env `esp32dev` (`espressif32@6.4.0`, Arduino, 115200 baud, `LED_BUILTIN=2`). No `lib_deps`: the Improv code is local.
- `src/main.cpp` - the app (about 450 lines): handles the Improv serial protocol itself using `improv.h`, connects to WiFi, serves a small page on port 80, and has antenna-selection functions for the ESP32-C6 (GPIO3 and GPIO14 control the onboard vs external antenna).
- `src/improv.cpp`, `src/improv.h` - copy of the Improv serial SDK. `lib/ImprovWiFiLibrary.h` - a local header for the library API.
- `library_patch.py` - PlatformIO pre-build script that patches the Improv WiFi Library source for ESP32-C6 (two small call changes). `merge_bin.py` - post-build script that merges bootloader, partitions and firmware into one image for a web flasher (offsets and `dio`/`40m`/`4MB` options are inside the script).
- Neither script is referenced in the committed `platformio.ini` (no `extra_scripts`), so they do not run unless that is added.

## Build / flash
- `pio run -t upload` and `pio device monitor` (115200). I found no tests and no CI.

## Gotchas
- The committed `main.cpp` has the antenna GPIO writes commented out ("disabled for ESP32 testing") and targets a plain ESP32. The author's working copy of this folder has a different, uncommitted C6 setup (board `esp32-c6-devkitm-1`, the Improv library as a dependency, antenna GPIO defines), so the C6 behaviour described above is not what the remote builds. Ask before assuming which version is current.
- Credentials are received at runtime over Improv; none are in the source.

## Deploy
- Push to `main`. There are no tags, releases or workflows.
