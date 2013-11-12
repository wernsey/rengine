#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
/* extern FILE *log_file; */

void log_init(const char *log_filename);

void log_deinit();

void sublog(const char *subsys, const char *fmt, ...);

void rlog(const char *fmt, ...);

void rerror(const char *fmt, ...);

void rwarn(const char *fmt, ...);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
