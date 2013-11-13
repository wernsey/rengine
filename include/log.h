#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* MISSING: Ability to set logging levels. At the moment everything is logged. */

extern FILE *log_file;

void log_init(const char *log_filename);

void log_deinit();

void sublog(const char *subsys, const char *fmt, ...);

void rlog(const char *fmt, ...);

void rerror(const char *fmt, ...);

void rwarn(const char *fmt, ...);

void sublog_ext(const char *file, int line, const char *subsys, const char *fmt, ...);

#define RLOG(msg) sublog_ext(__FILE__, __LINE__, "info", "%s", msg)
#define RERROR(msg) sublog_ext(__FILE__, __LINE__, "error", "%s", msg)
#define RWARN(msg) sublog_ext(__FILE__, __LINE__, "warn", "%s", msg)

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
