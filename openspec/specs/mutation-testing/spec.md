# Mutation Testing

## Purpose

Defines the mutation-testing quality bar for the project: the target mutation score, the Mull run timeout, and the minimum mutant-kill coverage expected across the main source modules.

## Requirements

### Requirement: Global mutation score reaches 85%

The system SHALL achieve a mutation score of at least 85% across all mutated source files, as reported by `mull-runner-18` via `make mutation-run` and verified by `make mutation-check`.

#### Scenario: Mutation report passes the threshold
- **WHEN** `make mutation-run` completes and generates `mutation_report.json`
- **THEN** the reported mutation score SHALL be greater than or equal to 85.0%
- **THEN** `make mutation-check` SHALL exit with code 0

### Requirement: Mutation run timeout is proportionate to test duration

The Mull run SHALL use a per-mutant timeout of 10 seconds (10000 ms) for both `--timeout` and `--minimum-timeout`, because the full unit suite completes in approximately 15 seconds and the slowest single test case is under 2 seconds.

#### Scenario: Hanging mutants are flagged within 10 seconds
- **WHEN** a mutant causes an infinite loop during mutation testing
- **THEN** the mutant SHALL be reported with status "Timeout" within 10 seconds
- **THEN** the mutation run SHALL NOT wait 60 seconds for a single hanging mutant

### Requirement: Function strategies exercise every result subtype

Every compiler function strategy SHALL have tests exercising the numeric, single-decimal, and double-decimal result subtype branches, so that subtype-comparison mutants in `application/compiler/functions/` do not survive solely because a branch is never executed.

#### Scenario: Function subtype branches are exercised
- **WHEN** a function strategy is invoked with numeric, single-decimal, and double-decimal arguments
- **THEN** each corresponding subtype branch SHALL be executed
- **THEN** the subtype-comparison mutants on those branches SHALL be killed

### Requirement: Statement strategies exercise omitted-argument branches

Compiler statement strategies SHALL have tests exercising the "omitted argument" (null subtype) branches, so that equality mutants guarding those branches do not survive because the null path is never taken.

#### Scenario: Omitted-argument branch is covered
- **WHEN** a statement with an optional argument is compiled without that argument
- **THEN** the null-subtype branch SHALL be executed
- **THEN** the equality mutant guarding that branch SHALL be killed

### Requirement: Scattered modules reach comparison coverage

The parser, lexer, domain, cli, and symbols export modules SHALL have tests exercising both sides of their `==`/`!=` guards and comparison boundaries, so that the currently surviving equality and boundary mutants in these modules are killed.

#### Scenario: Equality guards are covered on both sides
- **WHEN** a guard condition is tested with inputs that make it true and false
- **THEN** both branches SHALL be executed
- **THEN** the corresponding equality mutant SHALL be killed

### Requirement: Simplest resource-reader and codegen boundaries are covered

The simplest resource readers and the code-layout boundary checks in `compiler.cpp` and `rom.cpp` SHALL have tests covering exact boundary values (for example, a code block of exactly 16 KB), so that boundary-comparison mutants are killed.

#### Scenario: Exact boundary value is exercised
- **WHEN** an input produces a value exactly equal to a size boundary (e.g., 0x4000 bytes)
- **THEN** the boundary comparison SHALL be evaluated on the true side
- **THEN** the boundary-comparison mutant SHALL be killed

### Requirement: Existing tests remain unchanged

No existing unit test SHALL be modified or removed while raising the mutation score. New coverage SHALL be added through new test cases only.

#### Scenario: Existing suite is preserved
- **WHEN** new test cases are added
- **THEN** all previously passing tests SHALL continue to pass
- **THEN** the total test count SHALL increase, not decrease
