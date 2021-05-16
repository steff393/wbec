// Copyright (c) 2021 steff393, MIT license

#ifndef LOGGER_H
#define LOGGER_H

extern void logger_begin();
extern void logger_handle();

extern void log(uint8_t module, String msg, boolean newLine=true);
extern String log_time();

#endif /* LOGGER_H */
