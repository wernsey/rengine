#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef USESDL
#include <SDL2/SDL.h>
#endif

#include "log.h"

static FILE *log_file;

static void log_deinit();

#ifdef USESDL
static const char *category_name(int category) {
    switch(category) {
        case SDL_LOG_CATEGORY_APPLICATION   : return "Game   :";
        case SDL_LOG_CATEGORY_ERROR         : return "Error  :";
        case SDL_LOG_CATEGORY_SYSTEM        : return "System :";
        case SDL_LOG_CATEGORY_AUDIO         : return "Audio  :";
        case SDL_LOG_CATEGORY_VIDEO         : return "Video  :";
        case SDL_LOG_CATEGORY_RENDER        : return "Render :";
        case SDL_LOG_CATEGORY_INPUT         : return "Input  :";
        case LOG_CATEGORY_LUA               : return "Lua    :";
        case LOG_CATEGORY_MUSL              : return "Musl   :";
    }
    return "unknown:";
}

static const char *priority_name(SDL_LogPriority priority) {
    switch(priority) {
        case SDL_LOG_PRIORITY_VERBOSE   : return "VERBOSE :";
        case SDL_LOG_PRIORITY_DEBUG     : return "DEBUG   :";
        case SDL_LOG_PRIORITY_INFO      : return "INFO    :";
        case SDL_LOG_PRIORITY_WARN      : return "WARN    :";
        case SDL_LOG_PRIORITY_ERROR     : return "ERROR   :";
        case SDL_LOG_PRIORITY_CRITICAL  : return "CRITICAL:";
        default: return "UNKNOWN :";
    }
}

/* I had to replace the SDL logging function, because I'm using MinGW under 
Windows, so I don't have access to Visual Studio's debug output window. */
static void my_log_function(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    FILE *logfile = userdata;
    fputs(priority_name(priority), logfile);
    fputs(category_name(category), logfile);
    fputs(message, logfile);
    fputs("\n", log_file);
    if(priority > SDL_LOG_PRIORITY_INFO)
        fflush(logfile);
}
#endif

void log_init(const char *log_filename) {	
	log_file = fopen(log_filename, "w");
	if(!log_file) {
		fprintf(stderr, "error: Unable to open log file %s; using stdout\n", log_filename);
		log_file = stdout;
	}
    
#ifdef USESDL
    /* TODO: If you're porting this to mobile devices, you may not want to do this
    in order to benefit from the debuggers */
    SDL_LogSetOutputFunction(my_log_function, log_file);
#endif

	atexit(log_deinit);
}

static void log_deinit() {
	if(log_file != stdout)
		fclose(log_file);
}

#ifndef USESDL
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
#endif

void rlog(const char *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
#ifdef USESDL
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, fmt, arg);
#else
	inlog("info :", fmt, arg);
#endif
	va_end(arg);
}

void rerror(const char *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
#ifdef USESDL
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_WARN, fmt, arg);
#else
	inlog("error:", fmt, arg);
#endif
	va_end(arg);
}

void rwarn(const char *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
#ifdef USESDL
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_ERROR, fmt, arg);
#else
	inlog("warn :", fmt, arg);
#endif
	va_end(arg);
}
