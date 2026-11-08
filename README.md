# ZNetSuite

CP/M & RomWBW networking and small-web utility suite for Z80 systems.

## Requirements

- RomWBW repository (defaults to `../RomWBW` or configure via `ROMWBW=/path/to/RomWBW`).
- Host build environment with `make` and RomWBW host tools (`zxcc`, HI-TECH C).
- Optional: the `ttl2wifi` repository (`../ttl2wifi`) and Python 3.8+ for the SimH bridge emulator.

## Building

```sh
# Build all applications into ./bin/
make

# Or with custom RomWBW path
make ROMWBW=/path/to/RomWBW

# Clean build and bin artifacts
make clean
```

The project uses a strict **out-of-tree build** layout:
- All compilation, object files (`*.obj`), linking, and temporary CP/M emulator artifacts are confined exclusively to `build/` (e.g. `build/lib/`, `build/ztalk/`, `build/zgmi/`).
- The source tree (`include/`, `lib/`, `ztalk/`, `zgmi/`) contains **zero symlinks** and **zero compiler clutter**.
- Final executables are placed in `bin/`.

## Applications

- **`ztalk`**: Terminal bridge connecting the console (`CIO_CONSOLE`) to any RomWBW character I/O device (`CIO`).
- **`zgmi`**: Minimalist Gemini protocol client designed for systems paired with an AT-command serial-to-Wi-Fi bridge (such as `ttl2wifi` running on a Raspberry Pi Pico W). Features:
  - Dynamic VT100/ANSI terminal size detection via DSR Cursor Position Report (`\033[999;999H\033[6n`), with `Ctrl-L` to re-probe/redraw.
  - Vim scroll keybindings (`j`/`k`, `d`/`u`, `Ctrl-F`/`Ctrl-B`, `g`/`G`).
  - `Tab` to cycle between links with visual reverse-video highlights and auto-scroll into view.
  - `Enter` to follow links (`AT+GO=<id>`).
  - `b` to navigate back in history (`AT+BACK`) and `r` to reload.

## Common Libraries (`include/` & `lib/`)

Applications include only the specific headers they require:
- **`cio` ([`include/cio.h`](include/cio.h), [`lib/cio.c`](lib/cio.c))**: RomWBW Character I/O (`CIO`) and serial device driver library. Provides system detection, unit enumeration, hardware type naming, fast assembly `RST 8` character I/O primitives (`cio_ist`, `cio_ost`, `cio_in`, `cio_out`, `cio_puts`, `cio_flush`), line reader with timeout/abort, and AT modem command/response helpers.
- **`term` ([`include/term.h`](include/term.h), [`lib/term.c`](lib/term.c))**: High-speed, zero-overhead VT100/ANSI terminal handling backed directly by `CIO_CONSOLE`. Provides escape code macros, cursor and screen positioning, text attributes and colors, keystroke decoding (escape sequence parser for arrows, PgUp/PgDn, Home/End, Back-Tab), and dynamic terminal size detection via DSR Cursor Position Report (`\033[999;999H\033[6n`).
- **`fmt` ([`include/fmt.h`](include/fmt.h), [`lib/fmt.c`](lib/fmt.c))**: Text formatting, number presentation, and hex/integer parsing library. Provides hex parsing (`fmt_parse_hex`), auto-detected number parsing (`fmt_parse_num`), fast integer and hex rendering (`fmt_int`, `fmt_uint`, `fmt_hex8`, `fmt_hex16`), human-readable byte sizes (`fmt_size`), whitespace trimming (`fmt_trim`), case conversions (`fmt_lwr`, `fmt_upr`), and token extraction from the CP/M command tail at `0x0080` (`fmt_cpm_arg`).

## Bridge Emulator & SimH

The host-side `ttl2wifi` bridge emulator, the SimH runner and the small RomWBW patch needed for the
ACIA serial port now live in the **ttl2wifi** repository (`../ttl2wifi/sim/`, see its `README.md`).
It runs the same AT command code as the Pico W firmware.

```sh
make                          # build ZTALK.COM / ZGMI.COM into ./bin/
../ttl2wifi/sim/run_sim.sh    # boot RomWBW in SimH with the bridge on CIO unit 1
```

At the boot prompt type `2` for CP/M (`A>`), then run `ZTALK 01` or `ZGMI geminiprotocol.net`.

## Hardware Parity with SC131 Z180 Pocket Computer

Both `ztalk` and `zgmi` use 100% standard RomWBW HBIOS character I/O (`RST 8` with functions `BF_CIOIN`, `BF_CIOOUT`, `BF_CIOIST`, `BF_CIOOST`). The identical binary `.COM` files run without any changes on:
- **SimH AltairZ80**: `Unit 0` = SSER0 (Console), `Unit 1` = ACIA0 (TTL2WiFi bridge emulator).
- **SC131 Z180 Pocket Computer**: `Unit 0` = ASCI0 (Console Header A), `Unit 1` = ASCI1 (TTL2WiFi / Pico W Header B).

## AI Assistance

Artificial intelligence tools and coding assistants are actively used to help develop, refactor, and generate code, documentation, and build scripts across this project.

## Credits & License

- **Original HTalk**: Created by Tom Plano (`tomplano@proton.me`) for RomWBW.
- **RomWBW Toolchain & HBIOS**: Wayne Warthen and RomWBW contributors.
- **License**: GNU Affero General Public License v3 ([`LICENSE`](LICENSE)).
