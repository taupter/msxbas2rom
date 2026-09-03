# PRD - MSXBAS2ROM

## 1. Overview

MSXBAS2ROM is a cross-platform command-line compiler that converts MSX BASIC
source files into MSX ROM cartridge images. It targets MSX hardware and
emulators, producing both plain (16 KB) ROMs and segmented MegaROMs for
several mapper formats.

- **Language:** C++11
- **Platforms:** Linux, Windows, macOS
- **Deliverable:** CLI executable (`msxbas2rom`)
- **Methodology:** Spec Driven Development (SDD), DDD, Clean Code, SOLID, Clean Architecture
- **Quality:** TDD/BDD with coverage gates

## 2. Problem Statement

MSX developers who want to distribute BASIC programs as cartridges must
manually deal with tokenization, Z80 code generation, memory layout, mapper
banking, and resource embedding. MSXBAS2ROM automates this full path — from
BASIC text to a runnable ROM — while exposing a kernel of runtime helpers so
that generated code is compact and performs well on real hardware.

## 3. Goals

- Convert MSX BASIC source into valid, runnable ROM images.
- Support plain ROM and MegaROM formats (ASCII8, KonamiSCC, Konami4, ASCII16,
  ASCII16-X) with automatic or explicit mode selection.
- Embed external assets (DATA, IDATA, binary blobs, screen, sprite, Arkos, MTF)
  into the ROM with correct segment/address metadata.
- Emit debug symbol files for common toolchains.
- Keep the tool portable across Linux, Windows, and macOS.

## 4. Non-Goals

- A graphical IDE or full BASIC interpreter.
- Support for arbitrary third-party frameworks or unmaintained libraries.
- Runtime JIT or on-cartridge compilation (all code generation is offline).

## 5. Target Users

- MSX homebrew and game developers.
- Retro-computing enthusiasts distributing software on cartridge.
- Emulator users and tooling authors consuming exported symbol files.

## 6. Key Features

### 6.1 Compilation pipeline

The CLI runs four stages in order:

1. **Lexer** — tokenizes BASIC source into classified lexemes per line.
2. **Parser** — builds tag/action structures and validates syntax.
3. **Compiler** — performs semantic analysis and emits Z80 machine code,
   resolving symbols, fix-ups, RAM usage, and compiler-managed resources.
4. **ROM builder** — packages kernel, compiled code, resources, and symbols
   into the final ROM image.

### 6.2 Mapper formats

| Mode                     | Flag                | Layout                                   |
| ------------------------ | ------------------- | ---------------------------------------- |
| Plain ROM                | `-c` / `--plain`    | Single 16 KB page                        |
| ASCII8 (default MegaROM) | `-x` / `-8`         | 8 KB segments                            |
| KonamiSCC                | `-k` / `--scc`      | 8 KB segments                            |
| Konami4                  | `-4` / `--konami`   | 8 KB segments, no SCC                    |
| ASCII16                  | `-6` / `--ascii16`  | 16 KB pages                              |
| ASCII16-X                | `-7` / `--ascii16x` | 16 KB pages, 12-bit bank registers       |
| Auto                     | `-a` / `--auto`     | Fallback to ASCII8 on plain-ROM overflow |

### 6.3 Language support

- Standard BASIC statements and expressions (integers, floats, strings).
- Control flow including `ON ... GOTO/GOSUB` and event traps
  (`ON INTERVAL/SPRITE/STRIG/KEY/STOP`).
- Graphics (`LINE`, `BOX`, `PSET`, `CIRCLE`, `PAINT`, `COPY`, `SET PAGE`,
  `SET SCROLL`, tiles via `SET TILE`, `CMD MTF`).
- Sprite management (`SPRITE LOAD`, `PUT SPRITE`, `COLOR SPRITE`,
  `SET/GET SPRITE PATTERN/COLOR/FLIP/ROTATE`, `COLLISION()`).
- File I/O (`OPEN`, `CLOSE`, `INPUT#`, `LINE INPUT#`, `PRINT#`, `MAXFILES`,
  plus `EOF`, `LOC`, `LOF`, `FPOS`, `DSKF`, `ERR`).
- Resource directives (`DATA`, `IDATA`, `FILE`, `INCLUDE`).
- Debug symbol export (assembler, NoICE, OMDS, CDB, ELF).

### 6.4 Kernel dispatch

Generated code calls Z80 kernel routines through a stable word-pointer
dispatch table. The C++ side indexes it via contiguous `DISP_*` constants,
resolved at build time from the assembled kernel binary. This keeps the
compiler/kernel boundary symbolic and avoids hardcoded absolute addresses for
kernel-internal routines.

## 7. Architecture

Four layers with dependencies flowing inward:

- `src/cli` — entry point, argument parsing, diagnostics, orchestration.
- `src/application` — lexer, parser, compiler, builder, symbols.
- `src/domain` — shared structures (options, lexemes, nodes).
- `src/infrastructure` — filesystem, compression, logging, Z80 kernel assembly.

Behavioral polymorphism is centralized in the application layer via
Strategy/State plus Factory dispatch. See `doc/adr/ADR-001 MSXBAS2ROM.md` and
`openspec/specs/architecture/spec.md` for the full baseline.

## 8. Non-Functional Requirements

- **Portability:** builds and runs on Linux, Windows, and macOS.
- **Coverage:** ≥ 100% line coverage for medium/high criticality code,
  ≥ 90% for low criticality (overall target ≥ 85%).
- **Versioning:** semantic versioning `MAJOR.MINOR.PATCH.BUILD`, synced across
  source, changelog, Debian, and RPM metadata.
- **Security:** no `friend` keyword, no poorly understood third-party
  dependencies, security-by-design with vulnerability alerting.

## 9. Roadmap (Active Changes)

See `openspec/changes/` for the authoritative, current roadmap.

## 10. Out of Scope (Current)

- Full IDE/tooling integration beyond symbol export.
- Non-MSX target platforms.
- Runtime macro expansion or runtime `FN` machinery (macros are compile-time
  only).
