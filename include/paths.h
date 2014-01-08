#ifndef PATHS_H
#define PATHS_H
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

char *relpath(const char *from, const char *to, char *rel, size_t rellen);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
#endif /* PATHS_H */
