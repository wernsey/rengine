#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

FILE *log_file;

static void log_deinit();

void log_init(const char *log_filename) {	
	log_file = fopen(log_filename, "w");
	if(!log_file) {
		fprintf(stderr, "error: Unable to open log file %s; using stdout\n", log_filename);
		log_file = stdout;
	}
	atexit(log_deinit);
}

static void log_deinit() {
	if(log_file != stdout)
		fclose(log_file);
}

static void inlog(const char *subsys, const char *fmt, va_list arg) {
	fputs(subsys, log_file);
	fputs(": ", log_file);
	vfprintf(log_file, fmt, arg);
	fputs("\n", log_file);
	fflush(log_file);
}

void sublog(const char *subsys, const char *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
	inlog(subsys, fmt, arg);
	va_end(arg);
}

void rlog(const char *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
	inlog("info", fmt, arg);
	va_end(arg);
}

void rerror(const char *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
	inlog("error", fmt, arg);
	va_end(arg);
}

void rwarn(const char *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
	inlog("warn", fmt, arg);
	va_end(arg);
}

static void inlog_ext(const char *file, int line, const char *subsys, const char *fmt, va_list arg) {	
	fprintf(log_file, "%s:%s:%d: ", subsys, file, line);
	vfprintf(log_file, fmt, arg);
	fputs("\n", log_file);
	fflush(log_file);
}

void sublog_ext(const char *file, int line, const char *subsys, const char *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
	inlog_ext(file, line, subsys, fmt, arg);
	va_end(arg);
}

