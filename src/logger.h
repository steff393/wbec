// Copyright (c) 2021 steff393, MIT license

#ifndef LOGGER_H
#define LOGGER_H

// standard log
#define LOG(MODULE, TEXT, ...)    {char s[100]; snprintf_P(s, sizeof(s), PSTR(TEXT), __VA_ARGS__); log(MODULE, s);}
// standard log without newline
#define LOGN(MODULE, TEXT, ...)   {char s[100]; snprintf_P(s, sizeof(s), PSTR(TEXT), __VA_ARGS__); log(MODULE, s, false);}
// large log
#define LOGEXT(MODULE, TEXT, ...) {char s[600]; snprintf_P(s, sizeof(s), PSTR(TEXT), __VA_ARGS__); log(MODULE, s);};

extern void logger_allocate();
extern void logger_setup();
extern void logger_loop();

extern void     log(uint8_t module, String msg,      boolean newLine=true);
extern void     log(uint8_t module, const char *msg, boolean newLine=true);
extern String   log_time();
extern uint32_t log_unixTime();
extern String   log_uptime();

extern char* log_getBuffer(); 
extern void log_freeBuffer(); 

#endif /* LOGGER_H */
