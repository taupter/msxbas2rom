# ADR-001: Architectural Status of MSXBAS2ROM

## Status

Accepted.

This ADR records the current architectural state of MSXBAS2ROM as of
2026-09-03. It summarizes the architecture already described by the active
OpenSpec documents, mainly `openspec/specs/architecture/spec.md`, and should be
updated when that architectural baseline changes.

## Context

MSXBAS2ROM is a C++11 command-line compiler that converts MSX BASIC source files
into MSX ROM cartridge images. The tool validates CLI options, reads a BASIC
source file, performs lexical analysis, syntactic analysis, semantic/code
generation, and finally packages the generated code, kernel, symbols, and
resources into plain ROM or MegaROM output.

The project is being governed through Spec Driven Development. Active
requirements live under `openspec/specs/`, and active roadmap work lives under
`openspec/changes/`. The architecture spec requires DDD, Clean Code, SOLID, and
Clean Architecture principles, with source code organized into four main layers:

- `src/cli`
- `src/application`
- `src/domain`
- `src/infrastructure`

The codebase is in an incremental modernization state: legacy compiler behavior
is preserved while modules are being separated, covered by tests, and moved
toward explicit Strategy, State, and Factory based dispatch.

## Decision

MSXBAS2ROM will keep its current layered CLI compiler architecture and evolve it
incrementally instead of replacing the compiler pipeline. Architectural changes
must preserve the existing BASIC-to-ROM behavior, follow the OpenSpec
requirements, and prefer small refactors that separate responsibilities before
introducing new patterns.

The accepted architectural baseline is:

- The CLI is the executable delivery surface and owns argument parsing,
  user-facing diagnostics, informational commands, and pipeline orchestration.
- The application layer owns the compiler pipeline: lexer, parser, compiler,
  ROM builder, resource handling, and symbol export.
- The domain layer owns shared domain structures such as build options, lexemes,
  lexer lines, parser action/tag nodes, and symbol/fix/code nodes.
- The infrastructure layer owns low-level technical concerns such as the Z80
  kernel assembly, filesystem helpers, compression, and logging.
- Platform-specific runtime assets, especially the Z80 assembly kernel, remain
  isolated under `src/infrastructure/kernel/asm/`.
- Behavioral polymorphism is implemented through Strategy/State plus Factory
  dispatch where the project has already established that pattern.
- Build and test discovery remain path-driven so file moves and new source files
  are picked up automatically.

## Current Architecture

### Runtime Pipeline

The main runtime flow is linear and stage based:

1. `src/cli/main.cpp` creates build options and validates command-line input.
2. `Lexer` loads the BASIC source and produces tokenized line structures.
3. `Parser` consumes lexer lines and builds tag/action structures for BASIC
   statements, expressions, directives, resources, and program flow.
4. `Compiler` performs semantic analysis and emits Z80 machine code through the
   CPU opcode abstraction, while resolving symbols, fix-ups, RAM usage, and
   compiler-managed resources.
5. `Rom` packages the kernel, start page, compiled code, resource map, resource
   payloads, and optional symbol files into the final output artifact.

The CLI currently orchestrates these objects directly. This is acceptable for
the current architecture because the CLI remains thin and the major stage
behaviors are delegated to application services.

### Layer Responsibilities

`src/cli` contains the program entry point, CLI option parsing, app metadata,
Windows argument conversion support, and VS Code helper setup.

`src/application` contains the operational compiler behavior:

- `lexer/` tokenizes source text and uses line state helpers.
- `parser/` builds the syntax tree/action model and uses parser statement
  strategies.
- `compiler/` emits Z80 code, evaluates expressions, resolves symbols/fix-ups,
  and dispatches statement/function compilation through strategy factories.
- `builder/` creates ROM pages and resource maps, applies mapper-specific kernel
  patches, and writes ROM files.
- `symbols/` exports debug symbols through symbol export strategies.

`src/domain` contains shared data and concepts used across application stages,
including options, lexemes, parser nodes, and symbol/fix/code nodes.

`src/infrastructure` contains details that should not drive the application
model: filesystem support, compression, logging, and the assembly kernel. The
kernel is organized with `header.asm` as an orchestrator and domain-oriented
includes under `src/infrastructure/kernel/asm/src/header/`.

### ROM And Kernel Model

The builder supports plain ROM and MegaROM modes, including ASCII8, Konami SCC,
Konami4, ASCII16, and ASCII16-X. MegaROM mapper differences are handled by
patching the kernel through dispatch-table based addresses rather than broad
byte scanning.

Compiled C++ code calls kernel routines through a wrapper dispatch table and
`DISP_*` constants. The dispatch table must remain symbolic, contiguous, and
stable because the C++ side depends on the position of those entries.

### Resources And Symbols

Resources are compiler-visible artifacts managed by `ResourceManager` and
resource reader strategies. DATA, IDATA, binary blobs, packed blobs, screen,
sprite, Arkos, and MTF resources are integrated into the ROM layout with segment
and address metadata.

Debug symbol generation is handled by `SymbolManager` and export strategies for
the supported output formats. This keeps debugger/tooling output separate from
the compiler stage that discovers and resolves symbols.

### Testing And Build Model

The project uses Makefiles and C++11. Source and include discovery are mostly
path-driven through `find`, so new `.cpp` files and header directories are
included without maintaining large manual file lists.

Unit tests compile almost the full production tree except `src/cli/main.cpp`.
Integration tests exercise BASIC programs and ROM-generation behavior. Kernel
tests cover low-level assembly/runtime expectations. OpenSpec currently records
coverage goals and known areas that need expanded tests.

## Consequences

This architecture keeps the compiler understandable as a staged transformation
from BASIC text to ROM image. It also allows modernization to proceed gradually:
large orchestration classes can be thinned by extracting behavior into helpers,
strategies, and factories without changing generated ROM behavior.

The main cost is that the project still contains tightly coupled legacy areas,
especially around compiler internals, memory layout, fix-up resolution, and
kernel patching. Changes in these areas require careful tests because C++ code,
binary kernel layout, dispatch table indexes, and MSX runtime behavior are
interdependent.

Another consequence is that OpenSpec is part of the architecture, not just
documentation. New work should consult the relevant spec before changing code,
and architectural changes should update specs and ADRs together.

## Constraints And Guardrails

- Keep dependency direction inward: CLI and infrastructure may depend on
  application/domain concepts, but domain concepts should not depend on outer
  layers.
- Preserve C++11 compatibility and cross-platform behavior for Linux, Windows,
  and macOS.
- Do not introduce poorly understood or unmaintained third-party dependencies.
- Do not use the C++ `friend` keyword.
- Keep kernel assembly isolated under infrastructure.
- Preserve dispatch-table stability between C++ `DISP_*` constants and Z80
  kernel wrapper entries.
- Prefer the two-phase refactoring pattern: first separate files/classes without
  behavior changes, then introduce patterns with minimal logic changes.
- Do not manually update release changelog artifacts; use the project
  changelog/release workflow described by OpenSpec skills.

## Known Risks

- Compiler statement/function support is broad, and some high-complexity
  strategies still need stronger test coverage.
- ROM builder behavior depends on exact binary patch points in the assembled
  kernel.
- MegaROM code/resource segmentation and fix-up rewriting are central correctness
  points and should be treated as high-risk when modified.
- The CLI currently owns direct orchestration of all stages; this is acceptable
  today, but a future application-level pipeline service may be useful if the CLI
  grows or another frontend is introduced.

## References

- `openspec/specs/architecture/spec.md`
- `openspec/specs/governance/spec.md`
- `openspec/specs/glossary/spec.md`
- `openspec/specs/cli/spec.md`
- `openspec/specs/compiler/spec.md`
- `openspec/specs/builder/spec.md`
- `openspec/specs/kernel-call-routing/spec.md`
- `src/cli/main.cpp`
- `src/application/builder/rom.cpp`
- `src/application/compiler/compiler.cpp`
