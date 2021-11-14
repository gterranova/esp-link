#define ALEXA
char *alexaState(void);
#ifdef ALEXA
#ifndef CGIALEXA_H
#define CGIALEXA_H

#include "httpd.h"
int cgiAlexa(HttpdConnData *connData);

#endif // CGIALEXA_H
#endif // ALEXA
