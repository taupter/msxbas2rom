/***
 * @file test_compiler_functions.cpp
 * @brief MSXBAS2ROM compiler string/math function strategies unit testing
 */

// NOLINTBEGIN

#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include "compiler.h"
#include "compiler_context.h"
#include "compiler_statement_strategy_factory.h"
#include "cpu_workspace_context.h"
#include "doctest/doctest.h"
#include "fswrapper.h"
#include "lexer.h"
#include "logger.h"
#include "parser.h"
#include "vscode_helper.h"
#include "z80.h"

static std::string createTempBas(const std::string& filename,
                                 const std::string& content) {
  const std::string path = "tmp/" + filename;
  std::ofstream ofs(path);
  ofs << content;
  ofs.close();
  return path;
}

static std::string readFileContent(const std::string& path) {
  std::ifstream ifs(path);
  return std::string((std::istreambuf_iterator<char>(ifs)),
                     std::istreambuf_iterator<char>());
}

static std::string uniqueVscodeTargetDir() {
  static int counter = 0;
  return std::string("tmp/vscode_initialize_") + std::to_string(++counter);
}

static void removeVscodeScaffold(const std::string& target) {
  const std::string vscodeDir = pathJoin(target, ".vscode");
  std::remove(pathJoin(vscodeDir, "launch.json").c_str());
  std::remove(pathJoin(vscodeDir, "tasks.json").c_str());
  std::remove(pathJoin(vscodeDir, "debug.tcl").c_str());
  std::remove(pathJoin(vscodeDir, "sentinel.txt").c_str());
  std::remove(vscodeDir.c_str());
  std::remove(target.c_str());
}

static bool compileStatementProgram(const std::string& filename,
                                    const std::string& program,
                                    std::string* error_out = nullptr) {
  const std::string path = createTempBas(filename, program);

  shared_ptr<Z80OpcodeWriter> cpuOpcodeWriter = make_shared<Z80OpcodeWriter>();
  shared_ptr<Compiler> compiler = make_shared<Compiler>(cpuOpcodeWriter);
  shared_ptr<Lexer> lexer = make_shared<Lexer>();
  shared_ptr<Parser> parser = make_shared<Parser>();

  bool ok = false;
  if (lexer->load(path) && lexer->evaluate() && parser->evaluate(lexer)) {
    ok = compiler->build(parser);
  }

  if (!ok && error_out) {
    *error_out = compiler->getLogger()->errors().toString();
  }

  std::remove(path.c_str());

  return ok;
}

TEST_SUITE("CompilerStringFunctions") {
  TEST_CASE("MID$ function variants compile") {
    SUBCASE("MID$ with two arguments") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_mid2.bas", "10 A$=MID$(\"HELLO\",2)\n20 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("MID$ with three arguments") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_mid3.bas", "10 A$=MID$(\"HELLO\",2,3)\n20 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("MID$ with float start") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_mid_float.bas", "10 A$=MID$(\"HELLO\",2.5,3)\n20 END\n",
          &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }
  }

  TEST_CASE("INSTR function variants compile") {
    SUBCASE("INSTR with two arguments") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_instr2.bas", "10 A=INSTR(\"HELLO\",\"LL\")\n20 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("INSTR with three arguments") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_instr3.bas", "10 A=INSTR(1,\"HELLO\",\"LL\")\n20 END\n",
          &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }
  }

  TEST_CASE("STRING$ function variants compile") {
    SUBCASE("STRING$ with integer character code") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_string_int.bas", "10 A$=STRING$(5,65)\n20 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("STRING$ with string argument") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_string_str.bas", "10 A$=STRING$(5,\"A\")\n20 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }
  }

  TEST_CASE("USR function variants compile") {
    SUBCASE("USR0") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_usr0.bas", "10 DEF USR0=1\n20 A=USR0(5)\n30 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("USR1") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_usr1.bas", "10 DEF USR1=1\n20 A=USR1(5)\n30 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("USR9") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_usr9.bas", "10 DEF USR9=1\n20 A=USR9(5)\n30 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }
  }

  TEST_CASE("USING$ with complex formats compiles") {
    std::string errors;
    bool ok = compileStatementProgram(
        "fn_using_complex.bas",
        "10 PRINT USING$(\"+#,###.##^\", 12345.6)\n20 END\n", &errors);
    CHECK(ok == true);
    CHECK(errors.empty());
  }
}

TEST_SUITE("CompilerFunctionNumericSubtypes") {
  TEST_CASE("Math functions compile with numeric/single/double arguments") {
    struct MathCase {
      const char* name;
      const char* call;
    };

    const MathCase cases[] = {
        {"ABS", "ABS"},   {"INT", "INT"},   {"SGN", "SGN"},
        {"ATN", "ATN"},   {"COS", "COS"},   {"FIX", "FIX"},
        {"LOG", "LOG"},   {"RND", "RND"},   {"SQR", "SQR"},
        {"TAN", "TAN"},
    };

    for (const auto& test_case : cases) {
      SUBCASE(test_case.name) {
        SUBCASE("integer argument") {
          std::string errors;
          bool ok = compileStatementProgram(
              "fn_math_int.bas",
              "10 A=" + std::string(test_case.call) + "(2)\n20 END\n", &errors);
          CHECK(ok == true);
          CHECK(errors.empty());
        }

        SUBCASE("single decimal argument") {
          std::string errors;
          bool ok = compileStatementProgram(
              "fn_math_single.bas",
              "10 A=" + std::string(test_case.call) + "(2.5#)\n20 END\n",
              &errors);
          CHECK(ok == true);
          CHECK(errors.empty());
        }

        SUBCASE("double decimal argument") {
          std::string errors;
          bool ok = compileStatementProgram(
              "fn_math_double.bas",
              "10 A=" + std::string(test_case.call) + "(2.5)\n20 END\n",
              &errors);
          CHECK(ok == true);
          CHECK(errors.empty());
        }
      }
    }
  }

  TEST_CASE("String functions compile with numeric/single/double lengths") {
    SUBCASE("CHR$ with single and double decimal codes") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_chr_float.bas",
          "10 A$=CHR$(65.0#)\n20 B$=CHR$(66.5)\n30 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("HEX$/OCT$/BIN$ with decimal arguments") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_hex_float.bas",
          "10 A$=HEX$(255.0#)\n20 B$=OCT$(8.5)\n30 C$=BIN$(5.0#)\n40 END\n",
          &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("SPACE$/TAB with decimal arguments") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_space_float.bas",
          "10 A$=SPACE$(5.0#)\n20 B$=TAB(10.5)\n30 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("LEFT$/RIGHT$/MID$ with decimal lengths") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_left_float.bas",
          "10 A$=LEFT$(\"HELLO\",2.5#)\n20 B$=RIGHT$(\"HELLO\",3.5)\n"
          "30 C$=MID$(\"HELLO\",2.5#,4)\n40 END\n",
          &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("STRING$ with decimal count and char") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_string_float.bas",
          "10 A$=STRING$(5.0#,65.0#)\n20 B$=STRING$(4.5,\"X\")\n30 END\n",
          &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("INSTR with decimal start position") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_instr_float.bas",
          "10 A=INSTR(1.5#,\"HELLO\",\"LL\")\n20 B=INSTR(1.5,\"HELLO\",\"L\")\n"
          "30 END\n",
          &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }
  }

  TEST_CASE("I/O functions compile with decimal arguments") {
    SUBCASE("PEEK/VPEEK/IPEEK with decimal addresses") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_peek_float.bas",
          "10 A=PEEK(123.0#)\n20 B=VPEEK(456.5)\n30 C=IPEEK(789.0#)\n40 END\n",
          &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("INP/DSKF/EOF/FPOS/LOF with decimal arguments") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_io_float.bas",
          "10 A=INP(1.5#)\n20 B=DSKF(0.0#)\n30 C=EOF(1.5)\n"
          "40 D=FPOS(1.0#)\n50 E=LOF(1.5)\n60 END\n",
          &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("STICK/STRIG/INPUT$ with decimal arguments") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_stick_float.bas",
          "10 A=STICK(0.5#)\n20 B=STRIG(1.5)\n30 C$=INPUT$(5.0#)\n40 END\n",
          &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }
  }

  TEST_CASE("Graphics/basic/sound functions compile with decimal arguments") {
    SUBCASE("POINT/TILE with decimal coordinates") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_point_float.bas",
          "10 A=POINT(10.5#,20.5)\n20 B=TILE(5.0#,6.5)\n30 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("VDP/PAD/PLAY with decimal arguments") {
      std::string errors;
      bool ok = compileStatementProgram(
          "fn_vdp_float.bas",
          "10 A=VDP(1.5#)\n20 B=PAD(1.0#)\n30 C=PLAY(2.5)\n40 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }
  }
}

TEST_SUITE("CompilerSmokeTests") {
  TEST_CASE("TIME statement strategy smoke test") {
    SUBCASE("TIME read assignment") {
      std::string errors;
      bool ok = compileStatementProgram(
          "smoke_time_read.bas", "10 A=TIME\n20 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }

    SUBCASE("TIME write assignment") {
      std::string errors;
      bool ok = compileStatementProgram(
          "smoke_time_write.bas", "10 TIME=1\n20 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }
  }

  TEST_CASE("OPEN_GRP statement strategy smoke test") {
    SUBCASE("OPEN GRP for output") {
      std::string errors;
      bool ok = compileStatementProgram(
          "smoke_open_grp.bas",
          "10 OPEN \"GRP:\" FOR OUTPUT AS #1\n20 END\n", &errors);
      CHECK(ok == true);
      CHECK(errors.empty());
    }
  }

  TEST_CASE("VSCodeHelper smoke test") {
    SUBCASE("Constructs and exposes app filenames") {
      VSCodeHelper helper("msxbas2rom");
      CHECK(helper.getCompilerAppFilename() == "msxbas2rom");
      CHECK(helper.getEmulatorAppFilename().empty() == false);
    }

    SUBCASE("Initializes .vscode files in a target directory") {
      VSCodeHelper helper("msxbas2rom");

      const std::string target = uniqueVscodeTargetDir();
      removeVscodeScaffold(target);

      CHECK(createPath(target) == true);
      CHECK(helper.initializeInto(target) == true);

      const std::string vscodeDir = pathJoin(target, ".vscode");
      const std::string launchPath = pathJoin(vscodeDir, "launch.json");
      const std::string tasksPath = pathJoin(vscodeDir, "tasks.json");
      const std::string debugPath = pathJoin(vscodeDir, "debug.tcl");

      CHECK(fileExists(launchPath) == true);
      CHECK(fileExists(tasksPath) == true);
      CHECK(fileExists(debugPath) == true);

      CHECK(readFileContent(launchPath).find("\"version\"") !=
            std::string::npos);
      CHECK(readFileContent(tasksPath).find("\"label\": \"build\"") !=
            std::string::npos);
      CHECK(readFileContent(debugPath).find("proc main {}") !=
            std::string::npos);

      removeVscodeScaffold(target);

      CHECK(fileExists(launchPath) == false);
      CHECK(fileExists(tasksPath) == false);
      CHECK(fileExists(debugPath) == false);
      CHECK(pathExists(vscodeDir) == false);
      CHECK(pathExists(target) == false);
    }

    SUBCASE("Reports already initialized when target has .vscode") {
      VSCodeHelper helper("msxbas2rom");

      const std::string target = uniqueVscodeTargetDir();
      removeVscodeScaffold(target);

      CHECK(createPath(target) == true);

      const std::string vscodeDir = pathJoin(target, ".vscode");
      CHECK(createPath(vscodeDir) == true);

      const std::string sentinelPath = pathJoin(vscodeDir, "sentinel.txt");
      std::ofstream sentinel(sentinelPath);
      sentinel << "keep me";
      sentinel.close();

      CHECK(helper.initializeInto(target) == false);
      CHECK(readFileContent(sentinelPath) == "keep me");
      CHECK(fileExists(pathJoin(vscodeDir, "launch.json")) == false);
      CHECK(fileExists(pathJoin(vscodeDir, "tasks.json")) == false);
      CHECK(fileExists(pathJoin(vscodeDir, "debug.tcl")) == false);

      removeVscodeScaffold(target);
    }
  }
}

// NOLINTEND
