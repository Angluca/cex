#pragma once
#include "all.h"

#if defined(CEX_BUILD) || defined(CEX_NEW)

#    ifndef cexy$cex_self_cc
#        if defined(__clang__)
/// Macro constant derived from the compiler type used to initially build ./cex app
#            define cexy$cex_self_cc "clang"
#        elif defined(__GNUC__)
#            define cexy$cex_self_cc "gcc"
#        else
#            error "Compiler type is not supported"
#        endif
#    endif // #ifndef cexy$cex_self_cc

#    ifndef cexy$cc
#        define cexy$cc cexy$cex_self_cc
#    endif // #ifndef cexy$cc


#    ifndef cexy$cc_include
/// Include path for the #include "some.h" (may be overridden by user)
#        define cexy$cc_include "-I."
#    endif

#    ifndef cexy$build_dir
/// Build dir for project executables and tests (may be overridden by user)
#        define cexy$build_dir "./build"
#    endif

#    ifndef cexy$src_dir
/// Directory for applications and code (may be overridden by user)
#        define cexy$src_dir "./src"
#    endif

#    ifndef cexy$cc_args_sanitizer
/// Debug mode and tests sanitizer flags (may be overridden by user)
#        define cexy$cc_args_sanitizer                                                             \
            "-fsanitize-address-use-after-scope", "-fsanitize=address", "-fsanitize=undefined",    \
                "-fsanitize=leak", "-fstack-protector-strong"
#    endif

#    ifndef cexy$cc_args_release
/// Release mode compiler flags (may be overridden by user)
#        define cexy$cc_args_release "-Wall", "-Wextra", "-O2", "-g"
#    endif

#    ifndef cexy$cc_args_debug
/// Debug mode compiler flags (may be overridden by user)
#        define cexy$cc_args_debug "-Wall", "-Wextra", "-g3", cexy$cc_args_sanitizer
#    endif

#    ifndef cexy$test_cc_args
/// Test runner compiler flags (may be overridden by user)
#        define cexy$test_cc_args                                                                  \
            "-DCEX_TEST", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-g3",            \
                "-Itests/", cexy$cc_args_sanitizer
#    endif

#    ifndef cexy$test_launcher
#    define cexy$test_launcher
#endif

#    ifndef cexy$cex_self_args
/// Compiler flags used for building ./cex.c -> ./cex (may be overridden by user)
#        define cexy$cex_self_args
#    endif


#    ifndef cexy$ld_args
/// Linker flags (e.g. -L./lib/path/) (may be overridden by user)
#        define cexy$ld_args
#    endif

#    ifndef cexy$ld_libs
/// Linker libs  (e.g. -lm) (may be overridden by user)
#        define cexy$ld_libs
#    endif

#    ifndef cexy$debug_cmd
/// Command for running debugger for cex test/app debug  (may be overridden by
/// user)
#        define cexy$debug_cmd "gdb", "--args"
#    endif


#    ifndef cexy$process_ignore_kw
/**
@brief Pattern for ignoring extra macro keywords in function signatures (for cex process).
See `cex help str.match` for more information about patter syntax.
*/
#        define cexy$process_ignore_kw ""
#    endif


#    define cexy$cmd_process                                                                       \
        { .name = "process",                                                                       \
          .func = cexy.cmd.process,                                                                \
          .help = "Create CEX namespaces from project source code" }
#    define cexy$cmd_new { .name = "new", .func = cexy.cmd.new, .help = "Create new CEX project" }
#    define cexy$cmd_help                                                                          \
        { .name = "help",                                                                          \
          .func = cexy.cmd.help,                                                                   \
          .help = "Search cex.h and project symbols and extract help" }
#    define cexy$cmd_config                                                                        \
        { .name = "config",                                                                        \
          .func = cexy.cmd.config,                                                                 \
          .help = "Check project and system environment and config" }

#    define cexy$cmd_test                                                                          \
        { .name = "test",                                                                          \
          .func = cexy.cmd.simple_test,                                                            \
          .help = "Generic unit test build/run/debug" }

#    define cexy$cmd_app                                                                           \
        { .name = "app", .func = cexy.cmd.simple_app, .help = "Generic app build/run/debug" }

#    define cexy$cmd_all cexy$cmd_help, cexy$cmd_process, cexy$cmd_new, cexy$cmd_config

#    define cexy$initialize() cexy.build_self(argc, argv, __FILE__)

#    define cexy$description "\nCEX language (cexy$) build and project management system"
#    define cexy$usage " [-D] [-D<ARG1>] [-D<ARG2>] command [options] [args]"

// clang-format off
    #define cexy$epilog \
        "\nYou may try to get help for commands as well, try `cex process --help`\n"\
        "Use `cex -DFOO -DBAR config` to set project config flags\n"\
        "Use `cex -D config` to reset all project config flags to defaults\n"
// clang-format on

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
        "cex test run tests/test_file.c [--help]  - run test with passing arguments to the test runner program\n"


// clang-format on
struct __cex_namespace__cexy
{
    // Autogenerated by CEX
    // clang-format off

    void            (*build_self)(int argc, char** argv, const char* cex_source);
    bool            (*src_changed)(const char* target_path, arr$(char*) src_array);
    bool            (*src_include_changed)(const char* target_path, const char* src_path, arr$(char*) alt_include_path);
    char*           (*target_make)(const char* src_path, const char* build_dir, const char* name_or_extension, IAllocator allocator);

    struct {
        Exception       (*clean)(const char* target);
        Exception       (*create)(const char* target);
        Exception       (*find_app_target_src)(IAllocator allc, const char** target);
        Exception       (*run)(const char* target, bool is_debug, int argc, char** argv);
    } app;

    struct {
        Exception       (*config)(int argc, char** argv, void* user_ctx);
        Exception       (*help)(int argc, char** argv, void* user_ctx);
        Exception       (*new)(int argc, char** argv, void* user_ctx);
        Exception       (*process)(int argc, char** argv, void* user_ctx);
        Exception       (*simple_app)(int argc, char** argv, void* user_ctx);
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
