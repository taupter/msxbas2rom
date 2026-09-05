## 1. Core implementation (`initializeInto`)

- [x] 1.1 Declare `bool initializeInto(const std::string& path);` in `src/cli/vscode/vscode_helper.h` and verify the header still compiles (`make -C tests/unit build`)
- [x] 1.2 Implement `initializeInto(path)` in `src/cli/vscode/vscode_helper.cpp` so it creates `<path>/.vscode` via `createPath` and writes `launch.json`, `tasks.json`, `debug.tcl` under it, returning `false` without writing when the folder already exists or cannot be created; verify it returns `true`/`false` per the spec scenarios
- [x] 1.3 Make `initialize()` delegate to `initializeInto(".")` (current project folder) and verify the unit binary builds with no behavior change for existing callers

## 2. Hermetic test refactor

- [x] 2.1 In `tests/unit/src/test_compiler_functions.cpp`, replace the CWD-dependent `"Initializes .vscode files"` subcase of the `VSCodeHelper smoke test` with calls to `initializeInto()` against a unique directory under `tests/unit/tmp/`; verify it passes when run in isolation and does not require a pre-existing `tests/unit/.vscode`
- [x] 2.2 Add assertions that the three files are created with expected stable content markers (`"version"` in `launch.json`, `"label": "build"` in `tasks.json`, `proc main {}` in `debug.tcl`) and that cleanup removes the created `.vscode` artifacts; verify the subcase passes
- [x] 2.3 Add a hermetic "already initialized" scenario: pre-create the target `.vscode` with a sentinel file, expect `false` and the sentinel unchanged; verify it passes

## 3. CLI behavior verification

- [x] 3.1 Confirm `src/cli/main.cpp` requires no change and the `--vscode` flow still reports "already initialized" on failure and success otherwise; verify by building the release binary and running `msxbas2rom --vscode` twice in a scratch directory (second run returns exit code 1 with the already-initialized message)

## 4. Repair `openspec/specs/cli/spec.md`

- [x] 4.1 Rewrite `openspec/specs/cli/spec.md` into valid main-spec form (`## Requirements` section with `### Requirement:` headers), preserving the existing "Provide CLI that validates options and executes pipeline stages" text and its scenarios verbatim; verify `openspec validate --specs` no longer reports the file as structurally invalid
- [x] 4.2 Add a Requirement documenting the `--vscode` scaffolding initialization (the flag initializes the `.vscode` project files in the current project folder; when `.vscode` already exists it reports the project is already initialized and overwrites nothing), with scenarios; verify it cross-references the same behavior as `specs/vscode-scaffolding/spec.md` without duplicating the internal contract
- [x] 4.3 Audit the CLI flags/behaviors actually implemented in `src/cli/main.cpp` and `src/cli/options/build_options_setup.cpp` (and the archived `cli` deltas under `openspec/changes/archive/*/specs/cli/`) against the spec, and add Requirements with scenarios for any other implemented-but-undescribed behavior (e.g. symbols export flags `--symbol`/`-s`/`--noice`/`--omds`/`--cdb`/`--elf`, `-l`/`--lin`, `-i`/`-o` paths, `-q`/`-d`/`-D`/`-v`/`-H` informational flags); verify each requirement has at least one scenario
- [x] 4.4 Confirm the repaired spec validates cleanly (`openspec validate --specs` and `openspec doctor`) and that archiving this change would not be refused due to the `cli` target spec

## 5. Final verification

- [x] 5.1 Run the full unit suite (`make -C tests/unit run`) from a clean `tests/unit` (no `.vscode`) and again with a stale `tests/unit/.vscode` present; verify all previously passing tests still pass and the suite is green in both states
