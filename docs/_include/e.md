Symbol found at ./cex.h:335


 * @brief Non disposable assert, returns Error.assert CEX exception when failed
 

```c
#define e$assert(A)

#define e$assertf(A, format, ...)

#define e$except(_var_name, _func)

#define e$except_errno(_expression)

#define e$except_null(_expression)

#define e$except_silent(_var_name, _func)

#define e$except_true(_expression)

#define e$goto(_func, _label)

#define e$raise(return_uerr, error_msg, ...)

#define e$ret(_func)



#define e$assert (A)                                                                             \
        ({                                                                                          \
            if (unlikely(!((A)))) {                                                                 \
                __cex__fprintf(stdout, "[ASSERT] ", __FILE_NAME__, __LINE__, __func__, "%s\n", #A); \
                return Error.assert;                                                                \
            }                                                                                       \
        });

```
