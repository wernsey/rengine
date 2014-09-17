#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifdef _SDL_H
/* Some custom SDL log categories */
enum {
      LOG_CATEGORY_LUA = SDL_LOG_CATEGORY_CUSTOM,
      LOG_CATEGORY_MUSL,
};
#endif

/* MISSING: Ability to set logging levels. At the moment everything is logged. */

void log_init(const char *log_filename);

void rlog(const char *fmt, ...);

void rerror(const char *fmt, ...);

void rwarn(const char *fmt, ...);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
