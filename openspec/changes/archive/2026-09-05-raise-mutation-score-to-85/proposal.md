## Why

The mutation score is **65%** (1668 killed / 2549 mutants), below the 80% threshold already configured in `Makefile` (`--mutation-score-threshold 80`) and below the 85% project target. There are **874 surviving** and **7 timeout** mutants. Raising the score to ~85% improves confidence that the unit tests actually assert meaningful behavior rather than just execute lines.

## What Changes

- Add new unit tests (doctest) that kill surviving mutants, prioritized by kills-per-effort across:
  - **Function strategies** (`application/compiler/functions/`): exercise every `result[0]` subtype branch (numeric / single / double) currently left uncovered.
  - **Statement strategies** (`application/compiler/statements/`): exercise the `subtype_null` (omitted-argument) branches and boundary conditions.
  - **Scattered modules** (`parser`, `lexer`, `domain`, `cli`, `symbols`): cover `==` guards and comparison boundaries.
  - **Simplest survivors** in resource readers and `compiler.cpp`/`rom.cpp` (exact-size boundary cases).
- Adjust the Mull run timeout in `Makefile`: lower `--timeout 60000` / `--minimum-timeout 60000` to `10000` ms. This is a wall-clock/hygiene improvement only; it does not change the score (the 7 timeout mutants remain "Timeout").
- No production source changes are expected; this is a test-only change. The 7 timeout mutants are accepted as-is.
- Existing tests are **not** modified or removed (preserves the `unit-test-coverage` invariant).

## Capabilities

### New Capabilities

- `mutation-testing`: Defines the target mutation score (≥ 85%), the Mull timeout configuration, and the required mutant-kill coverage across modules.

### Modified Capabilities

<!-- No existing capability requirements change. -->

## Impact

- `tests/unit/src/*.cpp`: new test cases added across existing test files (compiler, parser, lexer, domain, resources, symbols, rom, fs, cli).
- `Makefile`: `mutation-run` target timeout flags changed from `60000` to `10000`.
- `scripts/check-mutation-score.py`: possibly update `THRESHOLD` from `80.0` to `85.0` (to match the new target).
- No production code, APIs, or dependencies change.
