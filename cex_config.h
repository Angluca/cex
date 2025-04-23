#ifdef CEX_DEBUG
#    define cexy$cex_self_args cexy$cc_args_sanitizer
#    define CEX_LOG_LVL 5 /* 0 (mute all) - 5 (log$trace) */
#else
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#endif

#ifdef CEX_TEST_NOASAN
#    define cexy$cc_args_sanitizer
#endif

#ifdef CEX_WINE
#    define cexy$cc "x86_64-w64-mingw32-gcc"
#    define cexy$cc_args_sanitizer
#    define cexy$test_launcher "wine"
#endif
