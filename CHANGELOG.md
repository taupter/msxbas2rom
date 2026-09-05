# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
with a MAJOR.MINOR.PATCH.BUILD scheme.

## [Unreleased]

### Added

- [add-16bit-segments-ascii16-4mb-ascii16x-8mb](openspec/changes/add-16bit-segments-ascii16-4mb-ascii16x-8mb) 16-bit segment support enabling ASCII16 up to 4MB and ASCII16X up to 8MB ROMs
- [add-define-def-fn-preprocessor](openspec/changes/add-define-def-fn-preprocessor) Compile-time macro support via DEFINE and DEF FN preprocessor
- [add-sprite-hitbox-margins](openspec/changes/add-sprite-hitbox-margins) Add per-sprite configurable collision hitbox margins via SET SPRITE HITBOX
- [fix-dim-position-order](openspec/changes/fix-dim-position-order) Fix DIM position order so array references before DIM work correctly
- [implement-double-via-float-float](openspec/changes/implement-double-via-float-float) Double precision operations via float emulation
- [refactor-resource-number-hl-protocol](openspec/changes/refactor-resource-number-hl-protocol) Resource number passing standardized to HL register protocol
- [set-page-screen4](openspec/changes/set-page-screen4) SET PAGE support for screen 4 compatibility

## [1.2.3.0] - 2026-09-05

### [2026-08-12-improve-code-coverage](openspec/changes/archive/2026-08-12-improve-code-coverage) Raise unit-test line coverage to ≥ 85% across the codebase

#### Added
- Add unit tests for the lowest-coverage compiler statements (SET, ON, COPY, PUT, SCREEN, CIRCLE, LET, PAINT, PSET, COLOR, KEY)
- Expand semantic-helper coverage (expression evaluator, variable emitter, code helper, float converter)
- Add tests for resource readers with little or no coverage (AKM reader, MTF map reader)
- Cover untested paths in `compiler.cpp` and `rom.cpp`
- Add tests for the graphics statement parser (graphics, put, on, set, screen)
- Add tests for low-coverage function strategies (MID$, INSTR$, STRING$, USING, USR)
- Add smoke tests for previously 0%-coverage files (`compiler_time_statement`, `compiler_open_grp_statement`, `vscode_helper`)
- Expand Z80 kernel (`z80.cpp`) coverage on unexercised paths

### [2026-09-05-raise-mutation-score-to-85](openspec/changes/archive/2026-09-05-raise-mutation-score-to-85) Raise the mutation score to ≥ 85% by killing surviving mutants with new unit tests

#### Added
- Add unit tests exercising every function-strategy result subtype (numeric, single, double)
- Add omitted-argument (null subtype) and boundary tests for statement strategies
- Add both-side equality and boundary tests across parser, lexer, domain, CLI, and symbols export modules
- Add exact-size boundary tests for resource readers and `compiler.cpp`/`rom.cpp`

#### Changed
- Raise the mutation-score threshold from 80% to 85% in the mutation run and check script
- Lower the Mull per-mutant timeout from 60000 ms to 10000 ms

### [2026-09-05-vscode-helper-initialize-into](openspec/changes/archive/2026-09-05-vscode-helper-initialize-into) Add VSCode scaffolding into a caller-chosen directory and repair the CLI spec

#### Added
- Add `VSCodeHelper::initializeInto(path)` to scaffold `.vscode` (`launch.json`, `tasks.json`, `debug.tcl`) under a target directory, reporting failure without writes when `.vscode` already exists or cannot be created

#### Changed
- Make `initialize()` delegate to `initializeInto(".")`, keeping the `--vscode` CLI behavior unchanged
- Refactor the CWD-dependent `VSCodeHelper` smoke test into hermetic tests under `tests/unit/tmp/` (content markers, cleanup, already-initialized scenario)
- Repair `openspec/specs/cli/spec.md` into valid main-spec form and document `--vscode` plus other implemented-but-undescribed CLI flags

[Unreleased]: https://github.com/amaurycarvalho/msxbas2rom/compare/v1.2.3.0...HEAD
[1.2.3.0]: https://github.com/amaurycarvalho/msxbas2rom/releases/tag/v1.2.3.0

See [CHANGELOG Archive](CHANGELOG-ARCHIVE.md) for older releases.
