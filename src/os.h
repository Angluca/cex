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
    union
    {
        Exc error;
        time_t mtime;
    };
    usize is_valid : 1;
    usize is_directory : 1;
    usize is_symlink : 1;
    usize is_file : 1;
    usize is_other : 1;
} os_fs_stat_s;
_Static_assert(sizeof(os_fs_stat_s) == sizeof(usize) * 2, "size?");

typedef Exception os_fs_dir_walk_f(const char* path, os_fs_stat_s ftype, void* user_ctx);

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

static const char* OSPlatform_str[] = {
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

static const char* OSArch_str[] = {
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
        const char* a = args[i];                                                                   \
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
        Exc result = os.cmd.run((const char**)args, _args_len, &_cmd);                             \
        if (result == EOK) {                                                                       \
            result = os.cmd.join(&_cmd, 0, NULL);                                                  \
        };                                                                                         \
        result;                                                                                    \
        /* NOLINTEND */                                                                            \
    })

#define os$cmd(args...)                                                                            \
    ({                                                                                             \
        const char* _args[] = { args, NULL };                                                      \
        usize _args_len = arr$len(_args);                                                          \
        os$cmda(_args, _args_len);                                                                 \
    })

#define os$path_join(allocator, path_parts...)                                                     \
    ({                                                                                             \
        const char* _args[] = { path_parts };                                                      \
        usize _args_len = arr$len(_args);                                                          \
        os.path.join(_args, _args_len, allocator);                                                 \
    })

struct __cex_namespace__os {
    // Autogenerated by CEX
    // clang-format off

    void            (*sleep)(u32 period_millisec);

    struct {
        Exception       (*create)(os_cmd_c* self, arr$(char*) args, arr$(char*) env, os_cmd_flags_s* flags);
        FILE*           (*fstderr)(os_cmd_c* self);
        FILE*           (*fstdin)(os_cmd_c* self);
        FILE*           (*fstdout)(os_cmd_c* self);
        bool            (*is_alive)(os_cmd_c* self);
        Exception       (*join)(os_cmd_c* self, u32 timeout_sec, i32* out_ret_code);
        Exception       (*kill)(os_cmd_c* self);
        char*           (*read_all)(os_cmd_c* self, IAllocator allc);
        char*           (*read_line)(os_cmd_c* self, IAllocator allc);
        Exception       (*run)(const char** args, usize args_len, os_cmd_c* out_cmd);
        Exception       (*write_line)(os_cmd_c* self, char* line);
    } cmd;

    struct {
        const char*     (*get)(const char* name, const char* deflt);
        Exception       (*set)(const char* name, const char* value);
    } env;

    struct {
        Exception       (*chdir)(const char* path);
        Exception       (*copy)(const char* src_path, const char* dst_path);
        Exception       (*dir_walk)(const char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx);
        arr$(char*)     (*find)(const char* path, bool is_recursive, IAllocator allc);
        char*           (*getcwd)(IAllocator allc);
        Exception       (*mkdir)(const char* path);
        Exception       (*mkpath)(const char* path);
        Exception       (*remove)(const char* path);
        Exception       (*remove_tree)(const char* path);
        Exception       (*rename)(const char* old_path, const char* new_path);
        os_fs_stat_s    (*stat)(const char* path);
    } fs;

    struct {
        char*           (*basename)(const char* path, IAllocator allc);
        char*           (*dirname)(const char* path, IAllocator allc);
        bool            (*exists)(const char* file_path);
        char*           (*join)(const char** parts, u32 parts_len, IAllocator allc);
        str_s           (*split)(const char* path, bool return_dir);
    } path;

    struct {
        OSArch_e        (*arch_from_str)(const char* name);
        const char*     (*arch_to_str)(OSArch_e platform);
        OSPlatform_e    (*current)(void);
        const char*     (*current_str)(void);
        OSPlatform_e    (*from_str)(const char* name);
        const char*     (*to_str)(OSPlatform_e platform);
    } platform;

    // clang-format on
};
__attribute__((visibility("hidden"))) extern const struct __cex_namespace__os os;

