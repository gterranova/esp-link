char *alexaState(void);
#ifdef ALEXA
#ifndef CGIALEXA_H
#define CGIALEXA_H

#include "httpd/httpd.h"
int cgiAlexa(HttpdConnData *connData);

#endif // CGIALEXA_H
#endif // ALEXA
