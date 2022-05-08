#ifndef SPIFFS_CGISPIFFS_H_
#define SPIFFS_CGISPIFFS_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <httpd/httpd.h>

void spiffs_init();
//int tplFPGA(HttpdConnData *connData, char * token, void ** arg);
int cgiDirectory(HttpdConnData *connData);
int cgiHandleFile(HttpdConnData *connData);
//int cgiUploadFile(HttpdConnData *connData);
//int cgiDelete(HttpdConnData *connData);
//int cgiSelectBoot(HttpdConnData *connData);

#if defined(__cplusplus)
}
#endif

#endif