// Copyright (c) 2021 steff393, MIT license

#ifndef MQTT_H
#define MQTT_H

extern void mqtt_begin();
extern void mqtt_handle();
extern void mqtt_publish(uint8_t i);
extern void mqtt_log(const char *output, const char *msg);

#endif /* MQTT_H */
