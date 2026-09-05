## Context

See `proposal.md` for the motivation. Current state relevant to this design:

- `VSCodeHelper::initialize()` (`src/cli/vscode/vscode_helper.cpp:59`) hardcodes `".vscode"` and calls `createPath(".vscode")` + three `write()` calls, all relative to the process CWD.
- `createPath` (`src/infrastructure/fs/fswrapper.cpp:68`) returns `MKDIR(path) == 0`; on Linux/Windows `mkdir` fails with `EEXIST` when the directory already exists, so `initialize()` returns `false` whenever `.vscode` is present in the CWD — the source of the environment-dependent failure.
- `pathJoin` (`fswrapper.cpp:52`) already normalizes empty parts and leading/trailing separators, and is what `initialize()` uses to build file names.
- `src/cli/main.cpp:117-137` drives `--vscode`: it treats a `false` return as "already initialized", prints an error, and returns exit code 1.
- The only direct unit coverage is the `VSCodeHelper smoke test` in `test_compiler_functions.cpp` (`CompilerSmokeTests`), whose `"Initializes .vscode files"` subcase runs against the CWD.

## Goals / Non-Goals

**Goals:**
- Decouple the scaffolding target directory from the process CWD so the write logic is testable in isolation.
- Keep the public, observable `--vscode` behavior byte-identical (create-or-report-already-initialized, no overwrites).
- Replace the CWD-dependent test subcase with hermetic coverage that does not depend on `tests/unit/.vscode` state.

**Non-Goals:**
- Not changing the generated `.vscode` content, templates, or the "already initialized" CLI semantics.
- Not adding recursive cleanup/overwrite of a pre-existing `.vscode` (out of scope; would change observable behavior).
- Not introducing dependencies or a new build/test framework.

## Decisions

### Decision 1: Add a target-directory operation and make the default delegate to it

Add `bool initializeInto(const std::string& path)` that builds `.vscode` under `path` and writes the three files, returning `false` (with no writes) when the `.vscode` folder cannot be created or already exists. `initialize()` becomes a one-line delegate calling `initializeInto(".")` (the current project folder).

- Rationale: single source of truth for the write logic; `initialize()` keeps exact current behavior because the CLI already runs from the project folder; the parameterized form is trivially testable from any directory.
- Alternative considered: keeping one method and temporarily `chdir`-ing in the test — rejected (mutates global process state and is brittle).
- Alternative considered: making `write()`/content public for tests — rejected (changes public surface for test convenience; unnecessary once the target dir is injectable).

### Decision 2: Preserve the fail-if-exists semantics in `initializeInto`

`initializeInto` keeps today's contract: if `<path>/.vscode` exists, or its creation fails, return `false` and write nothing.

- Rationale: the CLI message "already initialized" and the "do not overwrite" guarantee in the specs depend on this; it also makes the "already initialized" scenario directly testable hermetically (pre-create the folder, expect `false` + unchanged files).
- Alternative considered: idempotent overwrite — rejected, changes observable CLI behavior and contradicts the existing error path.

### Decision 3: Hermetic tests use a dedicated directory under `tests/unit/tmp/`

The refactored test creates a unique directory under `tests/unit/tmp/` (which already exists and is cleaned by the unit `Makefile`), calls the new operation there, asserts the three files exist with expected stable content markers, asserts the pre-existing-`.vscode` failure case leaves files untouched, then removes the artifacts it created.

- Rationale: `tmp/` is the test scratch area; content markers (`"version"`, `"label": "build"`, `proc main {}`) are stable even though the template strings are private static members.
- Note: file/dir helpers come from `fswrapper` (`createPath`, `fileExists`, `pathJoin`), already linked into the unit binary; guard `std::remove` cleanup so a stray leftover is removed before and after the subcase to keep it self-contained.

### Decision 4: Repair `openspec/specs/cli/spec.md` as an in-scope task

The main `cli` spec is structurally invalid: it holds delta headers (`## MODIFIED Requirements`) instead of a main-spec `## Requirements` section, so `openspec validate`/`archive` treat it as broken and its requirement is invisible to tooling. The repair is executed as part of this change (see tasks §4) and is a main-spec edit, not a new `cli` delta:

- Rewrite the file into valid main-spec form (`## Requirements` + `### Requirement:` headers), preserving the existing "Provide CLI that validates options and executes pipeline stages" content and its scenarios verbatim.
- Add a Requirement describing the `--vscode` scaffolding initialization (flag exists; default init targets the current project folder; already-initialized is reported and nothing is overwritten). The deeper behavioral contract stays in the new `vscode-scaffolding` capability; the `cli` requirement records the flag's externally visible behavior so the CLI spec is self-contained.
- Audit the flags actually implemented (`src/cli/main.cpp`, `src/cli/options/build_options_setup.cpp`) plus the archived `cli` deltas under `openspec/changes/archive/*/specs/cli/` and add any other implemented-but-undescribed behavior (symbols formats, `-l`/`--lin`, input/output paths, quiet/debug/doc/version/history handling) as Requirements with scenarios.

- Rationale: leaves the repo's specs consumable by `validate`/`archive`/`list`, documents currently invisible CLI behavior, and keeps the `--vscode` contract consistent with the new `vscode-scaffolding` capability.
- Alternative considered: leaving the repair to a separate future change — rejected because the malformed file blocks any future `cli` delta and the `--vscode` description naturally belongs with this work.
- Constraint: the `cli` spec edit stays a content/structure repair; no implementation behavior is changed by it.

## Risks / Trade-offs

- [Risk: concurrent change `raise-mutation-score-to-85` declares existing tests are not modified] → This change intentionally refactors one subcase to remove an environment-dependent failure; the refactor keeps/extends assertions and does not reduce coverage, so it must be reconciled if both changes are implemented against the same tree (apply this change's test edit after or coordinate order).
- [Risk: `initializeInto` with an empty or nonsense `path` behaves unexpectedly] → `pathJoin` already returns the relative part when the base is empty and handles separators; the spec only requires create-or-fail semantics, which hold regardless. No extra validation added (keep it minimal).
- [Risk: platform `mkdir` differences (Windows `_mkdir` lacks mode bits; `EEXIST` still returned)] → existing `createPath` abstraction already absorbs this; behavior verified on Linux CI + local run.
- [Risk: repairing `cli/spec.md` while the change's own delta lives under `vscode-scaffolding` could look like scope creep] → the task is a self-contained spec maintenance edit with explicit validation gates; it does not add a `cli` delta, so archiving this change is unaffected.

## Migration Plan

1. Implement `initializeInto(path)`; make `initialize()` delegate to it with `"."`.
2. Verify the `--vscode` CLI flow (success + already-initialized) is unchanged.
3. Refactor the fragile smoke-test subcase to the hermetic temp-dir form; add the already-initialized failure scenario.
4. Repair `openspec/specs/cli/spec.md` (valid structure, `--vscode` requirement, flags audit) and confirm `openspec validate --specs` passes.
5. Run `make -C tests/unit run`; confirm the suite is green from a clean `tests/unit` (no `.vscode` needed) and green with a pre-existing `tests/unit/.vscode`.

No rollback concern: the change is additive and behavior-preserving; reverting is a small diff.

## Open Questions

None — the target-directory semantics, the delegate default, the hermetic-test approach, and the `cli` spec repair are resolved.
