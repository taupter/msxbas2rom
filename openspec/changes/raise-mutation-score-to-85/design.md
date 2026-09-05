## Context

See `proposal.md` for motivation. Current state relevant to this design:

- Mutation score is **65%** (1668 killed / 2549 mutants; 874 survived + 7 timeout). Target ≥ 85% requires killing ~499 additional mutants.
- Tests use **doctest 2.4.11** in `tests/unit/src/*.cpp`, compiled into a single `test_unit` binary. `Makefile` `mutation-run` invokes `mull-runner-18` with `--timeout 60000 --minimum-timeout 60000`.
- Measured baseline: full unit suite runs in ~15s; slowest single test case is ~1.9s (only 2 cases exceed 1s). `doctest.h` is compiled with `DOCTEST_CONFIG_NO_MULTITHREADING`, so `doctest::timeout()` is not a hard watchdog (checked only at test end) and cannot interrupt an infinite loop.

## Goals / Non-Goals

**Goals:**
- Raise mutation score to ≥ 85% by adding tests only (no production code change).
- Reduce the Mull wall-clock waste caused by the 60s per-mutant timeout floor.
- Maximize kills-per-effort, prioritizing the easy categories first.

**Non-Goals:**
- Do NOT exclude files via `mull.yml excludePaths` or restrict mutators (e.g., drop `cxx_post_inc_to_post_dec`).
- Do NOT convert the 7 timeout mutants to "Killed"; they remain "Timeout" and are accepted.
- Do NOT modify or remove any existing test (preserves the `unit-test-coverage` invariant).

## Decisions

### Decision 1: Lower Mull timeout from 60000 to 10000 ms

Change `Makefile` `mutation-run` flags to `--timeout 10000 --minimum-timeout 10000`.

- Rationale: Mull's effective timeout is `max(baseline*10, minimum-timeout)`. With the floor at 60000, every hanging mutant wastes 60s (7 timeouts ≈ 7+ minutes). The full suite is 15s and the slowest test is 1.9s, so 10s is a 5x safety margin while flagging hangs 6x faster.
- Alternatives considered: 15000 (more conservative, still 4x faster than today); 5000 (risk of false "Timeout" on legitimately 3-5x slower mutants). Chose 10000 as the safe middle.
- Note: this does not change the score; it is wall-clock/hygiene only.

### Decision 2: Pure-test strategy, tiered by kills-per-effort

Kill mutants by adding test cases, ordered by difficulty. Recipe per tier (observed from the surviving mutants):

- **T1 — function strategies** (`test_compiler_functions.cpp`): survivors are almost all `cxx_eq_to_ne` on `result[0] == subtype_*` branches. Fix: invoke each strategy with numeric, single-decimal, and double-decimal arguments so every subtype branch executes. ~40 strategies, ~70 kills.
- **T2 — statement strategies** (`test_compiler_graphics.cpp`, `test_compiler_set.cpp`, `test_compiler_on.cpp`, `test_compiler_control.cpp`, `test_compiler.cpp`): survivors are `cxx_eq_to_ne` on `result_subtype == subtype_null` (omitted optional argument) plus boundary comparisons. Fix: compile statements with omitted optional arguments to hit the null branch; add boundary-value inputs. ~75 kills.
- **T5 — scattered** (`test_parser.cpp`, `test_lexer.cpp`, `test_domain.cpp`, `test_symbols.cpp`, `test_options.cpp`, `test_fs.cpp`): `cxx_eq_to_ne`/`cxx_ne_to_eq` guards and comparison boundaries. Fix: test both sides of each guard. ~70 kills.
- **T3 — simplest resource readers** (`test_resources.cpp`, `test_resources_extra.cpp`): `resource_manager.cpp`, `spr`, `csv`, `txt`, `data`, `string`, `blob` readers. Fix: byte-exact and boundary fixtures. ~40 kills. The complex binary parsers (`resource_akm_reader.cpp` 94 mutants, `resource_akx_reader.cpp` 25, `resource_mtf_map_reader.cpp` 24) are deferred to Phase 2.
- **T4 — simplest codegen boundaries** (`test_compiler.cpp`, `test_rom.cpp`): `build()` size boundaries (`>= 0x4000`) and `rom.cpp` layout arithmetic. Fix: inputs producing exactly 16KB. ~25 kills. The MegaROM `write()` fixup arithmetic (compiler.cpp lines 508-689) is deferred to Phase 2.

### Decision 3: Phase 2 for the hard remainder

Reaching 85% requires ~220 kills from the hard categories: order-insensitive `cxx_post_inc_to_post_dec` loops, the complex binary parsers (AKM/AKX/MTF), and MegaROM segment-fixup arithmetic. These are addressed only after Phase 1 (the easy ~280 kills) is confirmed, because they are expensive and their payoff per test is low.

### Decision 4: Update the score threshold to 85

Update `scripts/check-mutation-score.py` `THRESHOLD` from `80.0` to `85.0` (and `--mutation-score-threshold 80` in `Makefile` to `85`) so the CI/local gate matches the new target.

## Risks / Trade-offs

- [Risk: 85% may be unreachable via pure tests without touching hard modules] → Mitigation: Phase 1 gets to ~76%; Phase 2 targets the remainder. If a specific hard mutant proves untestable, surface it and consider a narrow `mull.yml` exclusion as a last resort (requires revisiting the proposal scope).
- [Risk: new tests make the suite slower, inflating the per-mutant baseline] → Mitigation: keep fixtures minimal; the 10s floor has 5x headroom over the current slowest test.
- [Risk: exact-boundary tests (16KB) are brittle] → Mitigation: assert the observable outcome (error message / code size), not raw byte arrays, wherever possible.
- [Trade-off: accepting 7 timeout mutants] → They cost 0.27% of score; not worth the effort to eliminate.

## Migration Plan

1. Add test cases tier by tier (T1 → T2 → T5 → T3 simple → T4 simple), verifying with the existing `bin/test_unit`.
2. Apply the `Makefile` timeout change and the score-threshold change.
3. User runs `make mutation-run` + `make mutation-check` manually to confirm each tier's contribution and the final score.

## Open Questions

None — the timeout value, tier ordering, and threshold are all resolved.
