
Simple console logging engine:

- Prints file:line + log type: `[INFO]    ( file.c:14 cexy_fun() ) Message format: ./cex`
- Supports CEX formatting engine
- Can be regulated using compile time level, e.g. `#define CEX_LOG_LVL 4`


Log levels (CEX_LOG_LVL value):

- 0 - mute all including assert messages, tracebacks, errors
- 1 - allow log$error + assert messages, tracebacks
- 2 - allow log$warn
- 3 - allow log$info
- 4 - allow log$debug (default level if CEX_LOG_LVL is not set)
- 5 - allow log$trace



```c
/// Log debug (when CEX_LOG_LVL > 3)
#define log$debug(format, ...)

/// Log error (when CEX_LOG_LVL > 0)
#define log$error(format, ...)

/// Log info  (when CEX_LOG_LVL > 2)
#define log$info(format, ...)

/// Log tace (when CEX_LOG_LVL > 4)
#define log$trace(format, ...)

/// Log warning  (when CEX_LOG_LVL > 1)
#define log$warn(format, ...)




```
