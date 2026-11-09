# Agent & Contributor Engineering Guide (`ZNetSuite`)

This document captures the architectural rules, compiler/linker constraints, memory management strategies, and terminal/serial lessons for working on **ZNetSuite** (RomWBW & CP/M-80 Z80 networking utilities). These guidelines are **harness-agnostic** and apply to any AI coding assistant or developer.

---

## 1. Toolchain & CP/M 8.3 Filename Constraints

- **Compiler & Environment:** Code is compiled with **HI-TECH Z80 C (CP/M-80) V3.09** executed on the host via `zxcc` (from the sibling `../RomWBW` repository).
- **Strict 8.3 Filenames:** Every `.c`, `.h`, `.obj`, `.lib`, and `.com` filename **must** fit within CP/M 8.3 limits (at most **8 characters** before the dot, **3 characters** after, e.g., `trm_scrl.c`, `cpm_bdos.c`, `cpmbdos.h`). Longer filenames are silently truncated by CP/M BDOS inside `zxcc`, breaking Makefile dependencies and `LIBR`/`LINQ`.

---

## 2. HI-TECH Z80 C Compiler & Object Size Limits

1. **Keep `.c` Translation Units Small (`ZAS.COM` Label Limit):**
   - HI-TECH C's assembler (`ZAS.COM`) has a fixed symbol table limit for compiler-generated temporary labels (`$CTMP1.$$$: Too many temporary labels`), which triggers around **350–450 lines** of control flow (`if`/`for`/`while`/`switch`).
   - Keep application `.c` files under ~300–400 lines by splitting distinct responsibilities into separate modules (e.g., `zgmi.c`, `ui.c`, `spool.c`).
   - Keep library `.c` files in `lib/` focused on a **single function** (or a tiny, tightly-coupled group of 20–80 lines) so `LINQ` only pulls in the exact object modules a binary needs.

2. **Keep Stack Frames Under 128 Bytes (`large offset (warning)`):**
   - On Z80, local variables are accessed via the `IX` frame pointer using a signed 8-bit displacement (`-128` to `+127`).
   - Declaring local stack arrays larger than ~64 bytes (e.g., `char line[160]`) pushes subsequent local variables past `-128`, causing HI-TECH C to emit `large offset (warning)` and generate slow multi-instruction address arithmetic.
   - **Rule:** Declare scratch buffers (`> 64` bytes) as `static` (or file-scope globals), and always declare scalar locals (`int`, `char *`, `unsigned char`) **before** any small local arrays at the top of a function.

3. **C Dialect & Syntax Rules:**
   - Use K&R function definitions (`func(a, b) int a; char *b; { ... }`) for consistency across the codebase.
   - Do **not** use `U` suffixes on integer literals (e.g., `8U` fails with `expression syntax`; use `(unsigned int)8` or bit shifts `<< 3`).
   - Prefer `for (;;)` over `while (1)` to avoid `constant relational expression (warning)`.
   - Wrap inline Z80 `#asm` / `#endasm` blocks in `/* clang-format off */` / `/* clang-format on */` so C formatters never alter Z80 mnemonics or labels.

---

## 3. Shared Library (`libznet.lib`) & Single-Pass Linker (`LINQ`)

1. **Zero `<stdio.h>` Dependency:**
   - Never include `<stdio.h>` (`printf`, `sprintf`, `fopen`, `fgets`, `sscanf`). HI-TECH C's `stdio` pulls in 4–6 KB of buffered stream I/O, heap allocation (`malloc`/`sbrk`), and Unix `errno` string tables.
   - Instead, use `ZNetSuite`'s lightweight primitives in `lib/`:
     - Console & Serial I/O: `cio_printf()`, `cio_puts()`, `cio_readline()`, `cio_in()`, `cio_out()` (`include/cio.h`)
     - Formatting & Parsing: `fmt_int()`, `fmt_uint()`, `fmt_hex8()`, `fmt_parse_num()`, `fmt_trim()`, `fmt_cpm_arg()` (`include/fmt.h`)
     - CP/M Disk File I/O: `cpm_bdos()`, `cpm_fcb_init()` (`include/cpmbdos.h`)
     - VT100/ANSI Terminal: `term_*()` (`include/term.h`)

2. **Topological Ordering in `lib/Makefile`:**
   - HI-TECH C's `LINQ` scans `.lib` archives in a **single pass**.
   - In `lib/Makefile`, `SRCS` **must** be ordered topologically from high-level callers down to low-level leaf primitives:
     - `trm_*.c` (calls `fmt_int`, `cio_*`) -> `hb_*.c` -> `fmt_*.c` (`fmt_pnum.c` *before* `fmt_phex.c` and `fmt_pint.c`) -> `cpm_*.c` (`cpm_fcb.c` *before* `cpm_bdos.c`) -> `cio_*.c` (`cio_cmd.c`, `cio_ok.c`, `cio_line.c` *before* `cio_to.c` *before* `cio_in.c`/`cio_out.c`/`cio_ist.c`).

3. **Move Common Functionality into `lib/`:**
   - Any routine useful to more than one utility (HBIOS device queries, CP/M BDOS/FCB wrappers, AT-modem framing, string/number formatting, ANSI terminal control) belongs in `lib/` as its own small `.c` file rather than duplicated in application directories.
   - Archiving common code into `libznet.lib` also keeps the `LINQ` command line short (avoiding CP/M's ~80-character command-tail truncation when `C.COM` invokes `LINQ`).

---

## 4. Makefile & Build Hygiene (`zxcc` Quirk)

- `zxcc` frequently returns exit status `0` to the host shell even when a CP/M tool (`C.COM`, `ZAS.COM`, `LINQ.COM`) fails with a fatal error.
- Because `HITECHSTATUS` in `rules.mk` checks `[ -s $@ ]` (whether the target `.obj` or `.com` file exists and is non-empty), **every compile and link rule in every `Makefile` must delete the target first**:
  ```makefile
  @rm -f $@ $(BUILD_DIR)/$*.OBJ
  ```
- Without `@rm -f`, a failed compilation leaves the previous build's `.obj` file in `build/`, causing `make` to falsely report `[ OK ]` and link stale object code.

---

## 5. CP/M Memory Management (32K vs. 62K TPA) & Disk Spooling

- **Variable TPA Size:** CP/M targets range from 32K RAM systems (TPA top around `0x7800`) to 62K TPA RomWBW systems (TPA top around `0xDC00`–`0xEE00`).
- **Avoid Large Static BSS Buffers:** HI-TECH C's startup (`NRTCPM.OBJ`) compares the end of BSS (`__Hbss`) against the CP/M BDOS base pointer at `0x0006`. If a static array pushes `__Hbss` above BDOS on a 32K machine, the program aborts on launch with `"Not enough memory to run."`
- **Dynamic TPA Sizing (`_Hbss` to `0x0006`):**
  - Keep static `text + data + bss` compact.
  - Place variable-sized document/stream buffers dynamically at `extern char _Hbss[];` up to `(*(unsigned int *)0x0006) - 1536` (reserving 1.5 KB below BDOS for the Z80 stack).
- **HBIOS vs. CP/M BDOS for Storage:**
  - Use **HBIOS CIO** (`BF_CIO*`) for all character/serial/console I/O.
  - Use **CP/M BDOS** (`0x0005`) for file I/O (`*.CFG`, `*.$$$` spool files). HBIOS disk calls (`BF_DIO*`) operate on raw physical LBA sectors with no filesystem and will corrupt mounted CP/M drives. Pointing CP/M spool files to the RomWBW RAM disk (`A:` or `B:`) achieves HBIOS banked-RAM speed safely through BDOS.
  - When transitioning a multi-extent CP/M 2.2 file from sequential writing (`BDOS_WRITE = 21`) to random record reading (`BDOS_RRAN = 33`), always call `BDOS_CLOSE (16)` and re-open with `BDOS_OPEN (15)` so all directory extents are committed.

---

## 6. Serial Line & VT100/ANSI UI Best Practices

1. **Eliminate Serial Flicker via Differential Rendering:**
   - Over 9600–38400 baud serial lines, a full 80×24 redraw takes 0.5–1.5 seconds.
   - Only redraw rows that actually changed (e.g., when moving link selection on the same page, only redraw the old link row, the new link row, and the status bar).
   - Use VT100 hardware scrolling regions (`\033[<top>;<bot>r` + `\033D` / `\033M`) for 1-line scrolling so only the single newly-revealed row is transmitted.
   - Keep the cursor hidden (`\033[?25l`) during navigation and redraws; only show it (`\033[?25h`) during inline text prompts and on program exit.
   - Use `\033[K` (`TERM_CLEAR_EOL`) instead of transmitting trailing space characters.

2. **Never Write to the Last Column (`term_cols`) of the Bottom Row (`term_rows`):**
   - Writing a character into column 80 of row 24 on a terminal with auto-wrap (`DECAWM`) causes the terminal to wrap and scroll the entire screen up by 1 line, pushing Row 1 off the top of the display.
   - Always clamp status bars and bottom-row messages to `term_cols - 1` characters.

3. **Multi-Byte Escape Sequence Timing (`term_getkey`):**
   - Over serial lines and in unthrottled Z80 simulators, the bytes of `\033[A`, `\033[3~` (`Delete`), `\033[5~` (`PgUp`), and `\033[6~` (`PgDn`) arrive asynchronously.
   - Always use a bounded polling loop (`term_wait_seq`) for every byte of an escape sequence—including the trailing `'~'`—and return `0` (not `KEY_ESC`) for unrecognized `\033[...` sequences so unknown keys never falsely trigger `Esc`/Cancel actions.

---

## 7. Executable Size Auditing, Link Map Profiling & Avoiding `zxcc` Busy Loops

1. **Mandatory Size Review After Adding Features:**
   - Whenever adding a feature or refactoring code increases a `.com` executable's size, **always** compare `ls -la bin/*.com` before and after, generate a `LINQ` link map, and actively look for size optimizations before finishing the task.
   - Check both **on-disk binary size** (`text + data`) and **static RAM footprint** (`__Hbss = text + data + bss`) in the link map to ensure 32K TPA compatibility is preserved.

2. **How to Avoid 100% CPU `zxcc` Busy Loops & Compiler Hangs:**
   - **Cause A (`OPTIM.COM` on `#asm` Files):** Never run HI-TECH C's peephole optimizer (`--O` / `OPTIM.COM`) on `.c` files containing `#asm` blocks:
     1. `OPTIM.COM`'s number parser enters an infinite 100% CPU loop inside `zxcc` when it encounters assembly hex constants like `0F8H` or `0005H`.
     2. Even with decimal constants, `OPTIM.COM`'s dead-register analysis does not know that `rst 8` (HBIOS) or `call 0005H` (BDOS) reads `BC`/`DE`, and will silently delete `ld b,...` / `ld c,...` instructions before the syscall!
     - **Rule:** Always compile `#asm` files with `HITECHASMOPTIONS = --N` (enforced via `if grep -q '#asm' $<; then OPT="$(HITECHASMOPTIONS)"; fi` in `Makefile`).
   - **Cause B (CP/M 127-Char Command Tail Overflow in `LIBR` & `LINQ`):** CP/M's command tail buffer at `0x0080` is strictly limited to **127 characters**. When a command line exceeds this limit inside `zxcc`, it is truncated; tools like `LIBR.COM` or `LINQ.COM` then drop into an interactive prompt loop waiting for console `stdin` inside the unthrottled Z80 emulator, burning 100% CPU indefinitely.
     - **Rule 1 (`LIBR`):** Never pass all `.obj` files to `LIBR.COM` in a single command. Always archive one `.obj` at a time in a shell loop (as in `lib/Makefile`: `$(ZXCC) $(LIBR) -r -libznet.lib -$$obj`).
     - **Rule 2 (`LINQ` Link Maps):** Never append `--Mfile.map` to `$(ZXCC) $(CC) --V --N ...`, because `C.COM` expands drive prefixes (`P:ZGMI.OBJ P:UI.OBJ ...`) and truncates the `LINQ` command line (`D: : Can't open`). Instead, invoke `LINQ` directly inside `build/<module>/` using short relative filenames and redirect `< /dev/null` so any unexpected prompt exits immediately:
       ```sh
       cd build/zgmi && $(ZXCC) $(HITECHDIR)/LINQ --Z --Mzgmi.map \
         --Ptext=0,data,bss --C100H --Ozgmi.com \
         $(HITECHDIR)/NRTCPM.OBJ zgmi.obj ui.obj spool.obj \
         ../lib/libznet.lib $(HITECHDIR)/LIBC.LIB < /dev/null
       ```
     - **Rule 3 (Clean Up Map Files):** Remove temporary `.map` files from `build/` once analysis is complete.

3. **Z80 Code & Data Size Optimization Checklist:**
   - **Compiler Optimizer (`HITECHOPTIONS = --O --N --D_ZNET=1` in `rules.mk`):**
     - Always keep `--O` enabled for pure-C files so `C.COM` runs `OPTIM.COM` and passes `-J` to `ZAS.COM` (converting 3-byte `JP` instructions into 2-byte `JR` relative branches, saving ~25–30% across all `text` segments).
     - Keep `--D_ZNET=1` in `HITECHOPTIONS`: in `C.COM` V3.09-19, adding `--O` inserts `OPTIM.COM` into `$$exec.$$$`, causing 2–5 character filenames (`zgmi.c`, `ui.c`, `spool.c`, `ztalk.c`) to land on a buggy 384-byte (`0x180`) sector boundary that truncates the `ZAS.COM` command (`Illegal switch` / `No file arguments`). The `--D_ZNET=1` define pads the `CPP` entry past the boundary so all 1–8 character filenames compile cleanly.
   - **Suppress `LIBC.LIB` Exit/FD/Errno Bloat (`lib/cpm_exit.c`):** `NRTCPM.OBJ` references `__initrsx` and `_exit`. Keep `lib/cpm_exit.c` in `libznet.lib` so `LINQ` resolves those symbols before scanning `LIBC.LIB`, preventing `exit.obj`, `cleanup.obj`, `close.obj`, `getuid.obj`, `bdos.obj`, and `sys_err.obj` (~1.2 KB of unused Unix FD tables and `errno` strings) from being linked.
   - **Strict Single-Function Granularity in `lib/`:** If a `.c` file in `lib/` contains two functions and a binary only needs one (e.g., `fmt_hex8` in `fmt_hex.c` vs. `fmt_hex16` in `fmt_hx16.c`, or `fmt_lwr` in `fmt_case.c` vs. `fmt_upr` in `fmt_upr.c`), split them into separate 8.3 `.c` files so `LINQ` never pulls dead functions into `.com` binaries.
   - **Minimize Z80 Call & Stack-Push Overhead:** Every C function call on Z80 pushes 2 bytes per argument plus a 3-byte `CALL` and stack-pointer adjustment.
     - Hoist common tail calls out of `if / else if` chains (e.g., call a UI redraw helper once at the bottom of an input loop instead of in every key branch).
     - Combine multi-line help/banner output into a single `cio_printf("...\n...\n")` string literal rather than issuing separate `cio_printf` calls per line.
     - Reuse existing `cio_printf` format specifiers (`%02X`, `%d`, `%s`) instead of pulling in standalone `fmt_hex8` buffer-formatting routines when formatting only for console output.


