#ifdef CEX_DEBUG
#    define cexy$cex_self_args cexy$cc_args_sanitizer
#    define CEX_LOG_LVL 5 /* 0 (mute all) - 5 (log$trace) */
#endif

#ifdef CEX_TEST_NOASAN
#    define cexy$cc_args_sanitizer
#endif
