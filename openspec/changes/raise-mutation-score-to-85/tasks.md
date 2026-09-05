## 1. Tooling and configuration

- [ ] 1.1 Lower Mull timeout in `Makefile` `mutation-run` from `--timeout 60000 --minimum-timeout 60000` to `--timeout 10000 --minimum-timeout 10000` and verify the target text is updated
- [ ] 1.2 Update `scripts/check-mutation-score.py` `THRESHOLD` from `80.0` to `85.0` and verify `make mutation-check` reports the new threshold
- [ ] 1.3 Update `Makefile` `--mutation-score-threshold 80` to `85` and verify the flag is present in `mutation-run`

## 2. T1 — Function strategies subtype coverage

- [ ] 2.1 Add numeric/single/double subtype tests for math functions (`abs`, `int`, `sgn`, `atn`, `cos`, `fix`, `log`, `rnd`, `sqr`, `tan`) in `test_compiler_functions.cpp` and verify `./bin/test_unit` passes
- [ ] 2.2 Add subtype tests for string functions (`chr`, `hex`, `oct`, `instr`, `mid`, `left`, `right`, `space`, `string`, `using`, `bin`, `tab`) in `test_compiler_functions.cpp` and verify `./bin/test_unit` passes
- [ ] 2.3 Add subtype tests for I/O functions (`dskf`, `eof`, `fpos`, `inp`, `input`, `lof`, `peek`/`vpeek`/`ipeek`, `stick`, `strig`) in `test_compiler_functions.cpp` and verify `./bin/test_unit` passes
- [ ] 2.4 Add subtype tests for graphics/basic/sound functions (`point`, `tile`, `vdp`, `pad`, `lpos`, `pos`, `maker`, `turbo`, `play`) in `test_compiler_functions.cpp` and verify `./bin/test_unit` passes

## 3. T2 — Statement strategies omitted-argument and boundary coverage

- [ ] 3.1 Add omitted-argument (null subtype) tests for graphics statements (`copy`, `line`, `put`, `pset`, `paint`, `circle`, `screen`, `color`, `key`, `get`, `sprite`) in `test_compiler_graphics.cpp` and verify `./bin/test_unit` passes
- [ ] 3.2 Add omitted-argument and boundary tests for `set` statements in `test_compiler_set.cpp` and verify `./bin/test_unit` passes
- [ ] 3.3 Add boundary tests for control statements (`on`, `interval`, `return`) in `test_compiler_on.cpp`/`test_compiler_control.cpp` and verify `./bin/test_unit` passes
- [ ] 3.4 Add omitted-argument tests for I/O statements (`print`, `out`, `poke`, `vpoke`, `wait`, `input`, `iread`, `close`, `call`, `bload`, `swap`, `ipoke`, `read`, `sound`, `play`, `cmd` handlers) in `test_compiler.cpp` and verify `./bin/test_unit` passes

## 4. T5 — Scattered module comparison coverage

- [ ] 4.1 Add both-side equality tests for parser statements (`data`, `def`, `file`, `generic`, `next`, `on`, `set`, `put`, `print`, `color`, `dim`, `if`, `graphics`) in `test_parser.cpp` and verify `./bin/test_unit` passes
- [ ] 4.2 Add both-side equality/boundary tests for lexer (`lexer_line_state`, `identifier_state`, `comment_state`, `keyword_state`, `operator_state`, `separator_state`, `lexer.cpp`) in `test_lexer.cpp` and verify `./bin/test_unit` passes
- [ ] 4.3 Add both-side equality tests for domain nodes (`lexeme`, `lexer_line_context`, `tag_node`, `action_node`) in `test_domain.cpp` and verify `./bin/test_unit` passes
- [ ] 4.4 Add boundary tests for symbols export strategies (`cdb`, `omds`, `noice`, `elf`, `symbol_file`) in `test_symbols.cpp` and verify `./bin/test_unit` passes
- [ ] 4.5 Add both-side equality tests for `cliparser` and `fswrapper` in `test_options.cpp`/`test_fs.cpp` and verify `./bin/test_unit` passes

## 5. T3 — Simplest resource-reader coverage

- [ ] 5.1 Add boundary/count tests for `resource_manager.cpp` (buildMap limits) in `test_resources.cpp` and verify `./bin/test_unit` passes
- [ ] 5.2 Add byte-exact and boundary tests for simple readers (`spr`, `csv`, `txt`, `data`, `string`, `blob` and `blob_chunk_packed`/`blob_packed`) in `test_resources.cpp`/`test_resources_extra.cpp` and verify `./bin/test_unit` passes

## 6. T4 — Simplest codegen boundary coverage

- [ ] 6.1 Add exact-16KB code-length boundary tests for `compiler.cpp` `build()` checks in `test_compiler.cpp` and verify `./bin/test_unit` passes
- [ ] 6.2 Add layout/equality tests for `rom.cpp` boundary arithmetic in `test_rom.cpp` and verify `./bin/test_unit` passes

## 7. Phase 2 — Hard remaining mutants

- [ ] 7.1 Add tests for order-insensitive `post_inc` loops in graphics/statement strategies in `test_compiler_graphics.cpp`/`test_parser.cpp` and verify `./bin/test_unit` passes
- [ ] 7.2 Add byte-exact fixtures for complex binary parsers (`akm`, `akx`, `mtf_map`) in `test_resources_extra.cpp` and verify `./bin/test_unit` passes
- [ ] 7.3 Add byte-exact MegaROM segment-fixup tests for `compiler.cpp` `write()` in `test_compiler.cpp` and verify `./bin/test_unit` passes

## 8. Final verification

- [ ] 8.1 Run the full unit suite (`make -C tests/unit all`) and verify all previously passing tests still pass and total test count increased
- [ ] 8.2 Run `make mutation-run` and `make mutation-check` (user-managed) and verify the mutation score reaches ≥ 85.0%
