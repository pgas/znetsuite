# ZNetSuite

CP/M & RomWBW networking and small-web utility suite for Z80 systems.

## Requirements

- RomWBW repository (defaults to `../RomWBW` or configure via `ROMWBW=/path/to/RomWBW`).
- Host build environment with `make` and RomWBW host tools (`zxcc`, HI-TECH C).
- Python 3.8+ on the host for the bridge simulator (`sim_bridge.py`).

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

## Software Bridge Emulator (`sim_bridge.py`)

[`sim_bridge.py`](sim_bridge.py) is a host-side software simulator for the Raspberry Pi Pico W `ttl2wifi` bridge. It implements the exact same Hayes AT and extended AT+ command set:

| Command | Description |
|---|---|
| `AT` | Attention check (returns `OK`) |
| `ATI` / `ATI0` | Bridge identity, IP, and settings |
| `AT?` / `AT+HELP` | Full command summary |
| `ATE0` / `ATE1` | Local echo off / on |
| `AT&K0` / `&K3` / `&K4` | Flow control mode (None / HW RTS-CTS / SW XON-XOFF) |
| `AT+PAGE=<lines>` | Screen pager (0 = off) |
| `AT+WSTAT` / `+WSCAN` | Wi-Fi status and access point scan |
| `AT+WDNS="host"` | DNS lookup |
| `AT+TIME` | Network UTC time sync |
| `AT+GEMINI="url"` | Real TLS Gemini fetch (numbers '=>' links `[ 1]`..`[ N]`) |
| `AT+GOPHER="url"` | Real TCP Gopher fetch (port 70, numbers links `[ 1]`..`[ N]`) |
| `AT+GO=<n>[,"qry"]` | Follow numbered link `n` from the current page |
| `AT+BACK` | Navigate back to previous page in URL history |
| `AT+LINKS` | Display list of numbered links from current page |
| `AT+HTTPGET="url"` | Real HTTP/HTTPS document fetch |

### Usage Modes

```sh
# 1. TCP Server (for SimH AltairZ80 SIO connection, default port 9999)
./sim_bridge.py --tcp 9999

# 2. Pseudo-terminal (creates a /dev/ttys* device pair for minicom or direct serial)
./sim_bridge.py --pty

# 3. Interactive stdio (type AT commands directly in terminal)
./sim_bridge.py --stdio
```

## Running in the RomWBW SimH Simulator

A single turnkey runner script [`sim/run_sim.sh`](sim/run_sim.sh) handles everything out-of-tree:
- Prepares an isolated working disk image in `build/sim/hd512.img` (never modifying master images).
- Automatically updates `ZTALK.COM` and `ZGMI.COM` onto the working disk.
- Spawns the background TTL2WiFi bridge emulator (`sim_bridge.py`) connecting to SimH `M2SIO0` on port 9999.
- Launches the `altairz80` simulator with dual serial support (`Char 0` SSER console, `Char 1` ACIA bridge).

```sh
./sim/run_sim.sh
```

At the boot prompt:
- Type `2` to boot the hard disk into CP/M (`A>`).
- Run `ZTALK 01` to test the bridge AT commands.
- Run `ZGMI geminiprotocol.net` to browse Gemini capsules.

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
