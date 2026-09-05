## Why

`VSCodeHelper::initialize()` writes `.vscode/` **relative to the process current working directory**, with no way to choose the target directory. This makes the behavior untestable in isolation: the unit test that exercises it depends on `.vscode` not pre-existing under the test working directory (`tests/unit/`), and once a stale `.vscode` exists (e.g. left over from an interrupted run), `mkdir` fails with `EEXIST` and `initialize()` returns `false`, leaving a permanently failing, environment-dependent test.

## What Changes

- Add a public `VSCodeHelper::initializeInto(const std::string& path)` that creates `<path>/.vscode` and writes `launch.json`, `tasks.json`, and `debug.tcl` into it, returning `false` (without modifying existing files) when the target directory already exists or cannot be created — preserving today's "already initialized" semantics.
- Make `initialize()` delegate to `initializeInto(".")`, i.e. the current project folder (the process working directory). The `--vscode` CLI flow and its error message remain unchanged.
- Refactor the fragile `"Initializes .vscode files"` subcase of the existing `VSCodeHelper smoke test` into hermetic coverage that calls `initializeInto()` against a dedicated temporary directory under `tests/unit/tmp/`, asserting the three files are created with expected content and cleaned up afterwards.
- Repair `openspec/specs/cli/spec.md`: it is structurally invalid (it contains delta headers such as `## MODIFIED Requirements` instead of a valid main-spec `## Requirements` section, so it is invisible to `validate`/`archive`). The repair converts it to valid main-spec form preserving the existing content, documents the missing `--vscode` scaffolding behavior, and audits the CLI flags actually implemented in `src/cli/main.cpp` / `src/cli/options/build_options_setup.cpp` against the spec, adding any other missing descriptions.
- No **BREAKING** change: existing callers (`src/cli/main.cpp` and the remaining smoke-test subcase using `initialize()`/getters) keep their current behavior.

## Capabilities

### New Capabilities

- `vscode-scaffolding`: Behavior of the VSCode MSX-BASIC project scaffolding — creating the `.vscode` project files (`launch.json`, `tasks.json`, `debug.tcl`) either into a caller-chosen directory or, by default, into the current project folder. (The existing `cli` main spec does not cover the `--vscode` scaffolding behavior, so this is new capability territory rather than a `cli` delta.)

### Modified Capabilities

<!-- none -->

## Impact

- `src/cli/vscode/vscode_helper.h` / `vscode_helper.cpp`: new `initializeInto(path)`; `initialize()` becomes a one-line delegate.
- `src/cli/main.cpp`: no functional change expected (verify `--vscode` flow still reports "already initialized" and success identically).
- `tests/unit/src/test_compiler_functions.cpp` (`CompilerSmokeTests`): the CWD-dependent subcase is replaced by hermetic `initializeInto()` assertions against `tests/unit/tmp/`.
- `openspec/specs/cli/spec.md`: structural repair plus new content (a `--vscode` requirement and any other implemented-but-undescribed CLI flags surfaced by the audit). Content basis for the audit: `src/cli/main.cpp`, `src/cli/options/build_options_setup.cpp`, and the archived `cli` deltas under `openspec/changes/archive/*/specs/cli/`.
- Note: the concurrently planned change `raise-mutation-score-to-85` declares "existing unit tests are not modified"; this change intentionally refactors one existing subcase to remove an environment-dependent failure. If both changes land, they should be reconciled (this refactor does not reduce assertion coverage).
