#ifndef ALEXA_H
#define ALEXA_H

#include "httpd/httpd.h"
//#define WEMO

typedef struct {
    char * name;
    bool state;
    uint8_t bri;
    uint16_t ct;
    uint32_t hue;
    uint16_t sat;
} fauxmoesp_device_t;

void alexa_init(void);
void initDiscovery(void);
int cgiHandleUpnpEvent(HttpdConnData *connData);
int cgiHandleApiEvent(HttpdConnData *connData);
int ICACHE_FLASH_ATTR tpl_hue_description(HttpdConnData *connData, char *token, void **arg);
void ICACHE_FLASH_ATTR customHttpdStartResponse(HttpdConnData *conn, int code, char* content_type, int len);
void ICACHE_FLASH_ATTR ExecuteCommandPower(int device, bool power, uint8_t bri);

#endif