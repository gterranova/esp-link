#ifndef HTTPD_H
#define HTTPD_H
#include <ip_addr.h>
#include <c_types.h>
#include <espconn.h>

#define HTTPD_CGI_MORE 0
#define HTTPD_CGI_DONE 1
#define HTTPD_CGI_NOTFOUND 2

typedef struct HttpdPriv HttpdPriv;
typedef struct HttpdConnData HttpdConnData;

typedef int (* cgiSendCallback)(HttpdConnData *connData);

//A struct describing a http connection. This gets passed to cgi functions.
struct HttpdConnData {
	struct espconn *conn;
	char *url;
	char *getArgs;
	const void *cgiArg;
	void *cgiData;
	HttpdPriv *priv;
	cgiSendCallback cgi;
};



//A struct describing an url that's not in the filesystem or otherwise available.
//Also the way cgi functions get connected to an URL.
typedef struct {
	const char *url;
	cgiSendCallback cgiCb;
	const void *cgiArg;
} HttpdBuiltInUrl;


void ICACHE_FLASH_ATTR httpdInit(HttpdBuiltInUrl *fixedUrls, int port);
const char *httpdGetMimetype(char *url);
void ICACHE_FLASH_ATTR httpdStartResponse(HttpdConnData *conn, int code);
void ICACHE_FLASH_ATTR httpdHeader(HttpdConnData *conn, const char *field, const char *val);
void ICACHE_FLASH_ATTR httpdEndHeaders(HttpdConnData *conn);

#endif