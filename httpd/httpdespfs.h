#ifndef HTTPDESPFS_H
#define HTTPDESPFS_H

#include <esp8266.h>
#include "espfs.h"
#include "espfsformat.h"
#include "cgi.h"
#include "httpd.h"

typedef struct {
	EspFsFile *file;
	void *tplArg;
	char token[64];
	int tokenPos;
	uint8 count;
} TplData;

typedef void (* TplCallback)(HttpdConnData *connData, char *token, void **arg);

int cgiEspFsHook(HttpdConnData *connData);
int cgiEspFsTemplate(HttpdConnData *connData);
//int ICACHE_FLASH_ATTR cgiEspFsHtml(HttpdConnData *connData);

#endif
