#pragma once
#include "all.h"

typedef struct os_cmd_flags_s
{
    u32 combine_stdouterr : 1; // if 1 - combines output from stderr to stdout
    u32 no_inherit_env : 1;    // if 1 - disables using existing env variable of parent process
    u32 no_search_path : 1;    // if 1 - disables propagating PATH= env to command line
    u32 no_window : 1;         // if 1 - disables creation of new window if OS supported
} os_cmd_flags_s;
_Static_assert(sizeof(os_cmd_flags_s) == sizeof(u32), "size?");

typedef struct os_cmd_c
{
    struct subprocess_s _subpr;
    os_cmd_flags_s _flags;
    bool _is_subprocess;
} os_cmd_c;

typedef struct os_fs_stat_s
{
    u64 is_valid : 1;
    u64 is_directory : 1;
    u64 is_symlink : 1;
    u64 is_file : 1;
    u64 is_other : 1;
    u64 size : 48;
    union
    {
        Exc error;
        time_t mtime;
    };
} os_fs_stat_s;
_Static_assert(sizeof(os_fs_stat_s) <= sizeof(u64) * 2, "size?");

typedef Exception os_fs_dir_walk_f(char* path, os_fs_stat_s ftype, void* user_ctx);

#define _CexOSPlatformList                                                                         \
    X(linux)                                                                                       \
    X(win)                                                                                         \
    X(macos)                                                                                       \
    X(wasm)                                                                                        \
    X(android)                                                                                     \
    X(freebsd)                                                                                     \
    X(openbsd)

#define _CexOSArchList                                                                             \
    X(x86_32)                                                                                      \
    X(x86_64)                                                                                      \
    X(arm)                                                                                         \
    X(wasm32)                                                                                      \
    X(wasm64)                                                                                      \
    X(aarch64)                                                                                     \
    X(riscv32)                                                                                     \
    X(riscv64)                                                                                     \
    X(xtensa)

#define X(name) OSPlatform__##name,
typedef enum OSPlatform_e
{
    OSPlatform__unknown,
    _CexOSPlatformList
    OSPlatform__count,
} OSPlatform_e;
#undef X

__attribute__((unused)) static const char* OSPlatform_str[] = {
#define X(name) #name,
    NULL,
    _CexOSPlatformList
#undef X
};

typedef enum OSArch_e
{
#define X(name) OSArch__##name,
    OSArch__unknown,
    _CexOSArchList OSArch__count,
#undef X
} OSArch_e;

__attribute__((unused)) static const char* OSArch_str[] = {
#define X(name) cex$stringize(name),
    NULL,
    _CexOSArchList
#undef X
};

#ifdef _WIN32
#define os$PATH_SEP '\\'
#else
#define os$PATH_SEP '/'
#endif

#if defined(CEX_BUILD) && CEX_LOG_LVL > 3
#define _os$args_print(msg, args, args_len)                                                        \
    log$debug(msg "");                                                                             \
    for (u32 i = 0; i < args_len - 1; i++) {                                                       \
        char* a = args[i];                                                                   \
        io.printf(" ");                                                                            \
        if (str.find(a, " ")) {                                                                    \
            io.printf("\'%s\'", a);                                                                \
        } else if (a == NULL || *a == '\0') {                                                      \
            io.printf("\'(empty arg)\'");                                                          \
        } else {                                                                                   \
            io.printf("%s", a);                                                                    \
        }                                                                                          \
    }                                                                                              \
    io.printf("\n");
#else
#define _os$args_print(msg, args, args_len)
#endif

#define os$cmda(args, args_len...)                                                                 \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        _Static_assert(sizeof(args) > 0, "You must pass at least one item");                       \
        usize _args_len_va[] = { args_len };                                                       \
        (void)_args_len_va;                                                                        \
        usize _args_len = (sizeof(_args_len_va) > 0) ? _args_len_va[0] : arr$len(args);            \
        uassert(_args_len < PTRDIFF_MAX && "negative length or overflow");                         \
        _os$args_print("CMD:", args, _args_len);                                                   \
        os_cmd_c _cmd = { 0 };                                                                     \
        Exc result = os.cmd.run(args, _args_len, &_cmd);                             \
        if (result == EOK) {                                                                       \
            result = os.cmd.join(&_cmd, 0, NULL);                                                  \
        };                                                                                         \
        result;                                                                                    \
        /* NOLINTEND */                                                                            \
    })

#define os$cmd(args...)                                                                            \
    ({                                                                                             \
        char* _args[] = { args, NULL };                                                      \
        usize _args_len = arr$len(_args);                                                          \
        os$cmda(_args, _args_len);                                                                 \
    })

#define os$path_join(allocator, path_parts...)                                                     \
    ({                                                                                             \
        char* _args[] = { path_parts };                                                      \
        usize _args_len = arr$len(_args);                                                          \
        os.path.join(_args, _args_len, allocator);                                                 \
    })

struct __cex_namespace__os {
    // Autogenerated by CEX
    // clang-format off

    Exc             (*get_last_error)(void);
    void            (*sleep)(u32 period_millisec);
    f64             (*timer)();

    struct {
        Exception       (*create)(os_cmd_c* self, char** args, usize args_len, os_cmd_flags_s* flags);
        bool            (*exists)(char* cmd_exe);
        FILE*           (*fstderr)(os_cmd_c* self);
        FILE*           (*fstdin)(os_cmd_c* self);
        FILE*           (*fstdout)(os_cmd_c* self);
        bool            (*is_alive)(os_cmd_c* self);
        Exception       (*join)(os_cmd_c* self, u32 timeout_sec, i32* out_ret_code);
        Exception       (*kill)(os_cmd_c* self);
        char*           (*read_all)(os_cmd_c* self, IAllocator allc);
        char*           (*read_line)(os_cmd_c* self, IAllocator allc);
        Exception       (*run)(char** args, usize args_len, os_cmd_c* out_cmd);
        Exception       (*write_line)(os_cmd_c* self, char* line);
    } cmd;

    struct {
        char*           (*get)(char* name, char* deflt);
        Exception       (*set)(char* name, char* value);
    } env;

    struct {
        Exception       (*chdir)(char* path);
        Exception       (*copy)(char* src_path, char* dst_path);
        Exception       (*copy_tree)(char* src_dir, char* dst_dir);
        Exception       (*dir_walk)(char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx);
        arr$(char*)     (*find)(char* path, bool is_recursive, IAllocator allc);
        char*           (*getcwd)(IAllocator allc);
        Exception       (*mkdir)(char* path);
        Exception       (*mkpath)(char* path);
        Exception       (*remove)(char* path);
        Exception       (*remove_tree)(char* path);
        Exception       (*rename)(char* old_path, char* new_path);
        os_fs_stat_s    (*stat)(char* path);
    } fs;

    struct {
        char*           (*abs)(char* path, IAllocator allc);
        char*           (*basename)(char* path, IAllocator allc);
        char*           (*dirname)(char* path, IAllocator allc);
        bool            (*exists)(char* file_path);
        char*           (*join)(char** parts, u32 parts_len, IAllocator allc);
        str_s           (*split)(char* path, bool return_dir);
    } path;

    struct {
        OSArch_e        (*arch_from_str)(char* name);
        char*           (*arch_to_str)(OSArch_e platform);
        OSPlatform_e    (*current)(void);
        char*           (*current_str)(void);
        OSPlatform_e    (*from_str)(char* name);
        char*           (*to_str)(OSPlatform_e platform);
    } platform;

    // clang-format on
};
__attribute__((visibility("hidden"))) extern const struct __cex_namespace__os os;

