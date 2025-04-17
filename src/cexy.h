#pragma once
#include "all.h"

#if defined(CEX_BUILD)

#ifndef cexy$cc
#if defined(__clang__)
#define cexy$cc "clang"
#elif defined(__GNUC__)
#define cexy$cc "gcc"
#else
# #error "Compiler type is not supported"
#endif
#endif // #ifndef cexy$cc

#ifndef cexy$cc_include
#define cexy$cc_include "-I."
#endif

#ifndef cexy$build_dir
#define cexy$build_dir "build/"
#endif

#ifndef cexy$cc_args
#define cexy$cc_args "-Wall", "-Wextra"
#endif

#ifndef cexy$ld_args
#define cexy$ld_args
#endif

#ifndef cexy$ld_libs
#define cexy$ld_libs
#endif

#ifndef cexy$debug_cmd
#define cexy$debug_cmd "gdb", "--args"
#endif


#ifndef cexy$process_ignore_kw
/**
@brief For ignoring extra macro keywords in function signatures of libraries, as str.match() pattern
string
*/
#define cexy$process_ignore_kw ""
#endif

#ifndef cexy$cc_args_test
#define cexy$cc_args_test                                                                          \
    "-DCEX_TEST", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-g3", "-Itests/",        \
        "-fsanitize-address-use-after-scope", "-fsanitize=address", "-fsanitize=undefined",        \
        "-fsanitize=leak", "-fstack-protector-strong"

#endif

#define cexy$cmd_process                                                                           \
    { .name = "process",                                                                           \
      .func = cexy.cmd.process,                                                                    \
      .help = "Create CEX namespaces from project source code" }
#define cexy$cmd_help                                                                              \
    { .name = "help",                                                                              \
      .func = cexy.cmd.help,                                                                       \
      .help = "Search cex.h and project symbols and extract help" }
#define cexy$cmd_config                                                                             \
    { .name = "config", .func = cexy.cmd.config, .help = "Check project and system environment and config" }

#define cexy$cmd_test                                                                             \
    { .name = "test", .func = cexy.cmd.simple_test, .help = "Test runner" }

#define cexy$cmd_app                                                                             \
    { .name = "app", .func = cexy.cmd.simple_test, .help = "App runner" }

#define cexy$cmd_all cexy$cmd_help, cexy$cmd_process, cexy$cmd_config

#define cexy$initialize() cexy.build_self(argc, argv, __FILE__)

#define cexy$description "Cex build system"

#define cexy$epilog                                                                                \
    "\nYou may try to get help for commands as well, try `cex process --help`\n"

// clang-format off
#define _cexy$cmd_test_help (\
        "CEX built-in simple test runner\n"\
        "\nEach cexy test is self-sufficient and unity build, which allows you to test\n"\
        "static funcions, apply mocks to some selected modules and functions, have more\n"\
        "control over your code. See `cex config --help` for customization/config info.\n"\
\
        "\nCEX test runner keep checking include modified time to track changes in the\n"\
        "source files. It expects that each #include \"myfile.c\" has \"myfile.h\" in \n" \
        "the same folder. Test runner uses cexy$cc_include for searching.\n"\
\
        "\nCEX is a test-centric language, it enables additional sanity checks then in\n" \
        "test suite, all warnings are enabled -Wall -Wextra. Sanitizers are enabled by\n"\
        "default.\n"\
\
        "\nCode requirements:\n"\
        "1. You should include all your source files directly using #include \"path/src.c\"\n"\
        "2. If needed provide linker options via cexy$ld_libs / cexy$ld_args\n"\
        "3. If needed provide compiler options via cexy$cc_args_test\n"\
        "4. All tests have to be in tests/ folder, and start with `test_` prefix \n"\
        "5. Only #include with \"\" checked for modification \n"\
\
        "\nTest suite setup/teardown:\n"\
        "// setup before every case  \n"\
        "test$setup_case() {return EOK;} \n"\
        "// teardown after every case \ntest$setup_case() {return EOK;} \n"\
        "// setup before suite (only once) \ntest$setup_suite() {return EOK;} \n"\
        "// teardown after suite (only once) \ntest$setup_suite() {return EOK;} \n"\
\
        "\nTest case:\n"\
        "\ntest$case(my_test_case_name) {\n"\
        "    // run `cex help tassert_` / `cex help tassert_eq` to get more info \n" \
        "    tassert(0 == 1);\n"\
        "    tassertf(0 == 1, \"this is a failure msg: %d\", 3);\n"\
        "    tassert_eq(buf, \"foo\");\n"\
        "    tassert_eq(1, true);\n"\
        "    tassert_eq(str.sstr(\"bar\"), str$s(\"bar\"));\n"\
        "    tassert_ne(1, 0);\n"\
        "    tassert_le(0, 1);\n"\
        "    tassert_lt(0, 1);\n"\
        "    return EOK;\n"\
        "}\n"\
        \
        "\nIf you need more control you can build your own test runner. Just use cex help\n"\
        "and get source code `./cex help --source cexy.cmd.simple_test`\n")
        
#define _cexy$cmd_test_epilog \
        "\nTest running examples: \n"\
        "cex test create tests/test_file.c        - creates new test file from template\n"\
        "cex test build all                       - build all tests\n"\
        "cex test run all                         - build and run all tests\n"\
        "cex test run tests/test_file.c           - run test by path\n"\
        "cex test debug tests/test_file.c         - run test via `cexy$debug_cmd` program\n"\
        "cex test clean all                       - delete all test executables in `cexy$build_dir`\n"\
        "cex test clean test/test_file.c          - delete specific test executable\n"\
        "cex test run tests/test_file.c [--help]  - run test with passing arguments to the test runner program\n"\


// clang-format on
struct __cex_namespace__cexy {
    // Autogenerated by CEX
    // clang-format off

    void            (*build_self)(int argc, char** argv, const char* cex_source);
    bool            (*src_changed)(const char* target_path, arr$(char*) src_array);
    bool            (*src_include_changed)(const char* target_path, const char* src_path, arr$(char*) alt_include_path);
    char*           (*target_make)(const char* src_path, const char* build_dir, const char* name_or_extension, IAllocator allocator);

    struct {
        Exception       (*config)(int argc, char** argv, void* user_ctx);
        Exception       (*help)(int argc, char** argv, void* user_ctx);
        Exception       (*process)(int argc, char** argv, void* user_ctx);
        Exception       (*simple_test)(int argc, char** argv, void* user_ctx);
    } cmd;

    struct {
        Exception       (*clean)(const char* target);
        Exception       (*create)(const char* target, bool include_sample);
        Exception       (*make_target_pattern)(const char** target);
        Exception       (*run)(const char* target, bool is_debug, int argc, char** argv);
    } test;

    struct {
        Exception       (*make_new_project)(const char* proj_dir);
    } utils;

    // clang-format on
};
__attribute__((visibility("hidden"))) extern const struct __cex_namespace__cexy cexy;

#endif // #if defined(CEX_BUILD)
