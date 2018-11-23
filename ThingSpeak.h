#include "mbed.h"
#include "http_request.h"
#include "ESP32Interface.h"

extern void ThingSpeak_ChannelUpdate(int field1, int field2);
extern int ThingSpeak_init();
