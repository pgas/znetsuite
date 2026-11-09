# ZNetSuite

CP/M & RomWBW networking and small-web utility suite for Z80 systems.

Despite the name, they are all serial applications and need to be connected to the net through a serial bridge such as https://github.com/pgas/ttl2wifi.

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

## Applications

- **`ztalk`**: partial rewrite of RomWBW HTalk in C.
- **`zgmi`**: Gemini protocol client.
- **`ztelnet`**: RFC 854 Telnet client for Hayes `ATDT` serial modems.

## AI Assistance

Artificial intelligence tools and coding assistants are actively used to help develop, refactor, and generate code, documentation, and build scripts across this project.

## Credits & License

- **Original HTalk**: Created by Tom Plano (`tomplano@proton.me`) for RomWBW.
- **RomWBW Toolchain & HBIOS**: Wayne Warthen and RomWBW contributors.
- **License**: GNU Affero General Public License v3 ([`LICENSE`](LICENSE)).
