#include <esp8266.h>
#include "cgi.h"
#include "config.h"
#include "esp8266_spiffs.h"
#include "cgispiffs.h"

#define NOTICE(format, ...) do {                                            \
  os_printf(format "\n", ## __VA_ARGS__);                                   \
} while ( 0 )

void ICACHE_FLASH_ATTR spiffs_init()
{
    init();
}

ICACHE_FLASH_ATTR u32_t free_space()
{
    return get_free_space();
}

int ICACHE_FLASH_ATTR cgiDirectory(HttpdConnData *connData) {
    if (connData->conn==NULL) {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }
    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type", "text/plain");
    //httpdHeader(connData, "Content-Length", "9");
    httpdEndHeaders(connData);

    char buf[256];

    struct spiffs_dirent *pe;

    os_snprintf(buf, sizeof(buf), 
            "{\"info\":{\"total\":%u,\"used\":%u},"
            "\"files\":[", get_total_size(), get_used_size());
    httpdSend(connData, buf, -1);

    int first = 1;
    while ((pe = list(first? 0: 1))) {
        os_snprintf(buf, sizeof(buf), 
                "%s{\"fsid\":%u,\"name\":\"%s\",\"size\":%u}", 
                first ? "" : ",", pe->obj_id, pe->name, pe->size);
        httpdSend(connData, buf, -1);
        first = 0;
    }
    httpdSend(connData, "]}", -1);

    return HTTPD_CGI_DONE;
}

static const char *httpNotFoundHeader = "HTTP/1.0 404 Not Found\r\nConnection: close\r\n"
  "Content-Type: text/plain\r\nContent-Length: 12\r\n\r\nNot Found.\r\n";

typedef struct {
	spiffs_file fd;
	int sent;
    int total;
} TplData;

int ICACHE_FLASH_ATTR handleFile(HttpdConnData *connData) {
	TplData *tpd=connData->cgiData;

    int len = 0;
    char buffer[1024];

	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
        if (tpd != NULL) {
    		close(tpd->fd);
            os_free(tpd);
        }
		return HTTPD_CGI_DONE;
	}

	if (tpd==NULL) {
		//First call to this cgi. Open the file so we can read it.

        char *filename = os_strstr(connData->url, "spiffs/")+6;
        if ((*(filename+1) == '\0')) return cgiDirectory(connData);
        else if (!file_exists(filename)) {
            //NOTICE("%s not found. 404!\n", connData->url);
            httpdSend(connData, httpNotFoundHeader, -1);
            httpdFlush(connData);
            return HTTPD_CGI_DONE;
        }
        int size = file_size(filename);

		tpd = (TplData *)os_malloc(sizeof(TplData));
		tpd->fd = open(filename);
        tpd->sent = 0;
        tpd->total = size;
		connData->cgiData=tpd;

        //char fileSize[10];
        //os_snprintf(fileSize, sizeof(fileSize), "%d", file_size(filename));

        httpdStartResponse(connData, 200);
        httpdHeader(connData, "Content-Type", httpdGetMimetype(connData->url));
        //httpdHeader(connData, "Content-Length", fileSize);
        httpdEndHeaders(connData);
        return HTTPD_CGI_MORE;
    }

    len = n_read(tpd->fd, &buffer[0], 1024);
    if (len==0) {
        return HTTPD_CGI_DONE;
    } else if (httpdSend(connData, buffer, len)) {
        if (len!=1024) {
            //We're done.
            close(tpd->fd);
            os_free(tpd);
            return HTTPD_CGI_DONE;
        }
    }
    
    return HTTPD_CGI_MORE;
}

int ICACHE_FLASH_ATTR cgiHandleFile(HttpdConnData *connData) {
    if (connData->conn==NULL) {
        //Connection aborted. Clean up.
        return HTTPD_CGI_DONE;
    }

    //if (connData->requestType == HTTPD_METHOD_POST || connData->requestType == HTTPD_METHOD_PUT ) {
    //    return handleUpdate(connData);
    //} 
    //else 
    if (connData->requestType == HTTPD_METHOD_GET) {
        return handleFile(connData);
    }
    return HTTPD_CGI_DONE;
}
/*

// general template parameters
int ICACHE_FLASH_ATTR tplFPGA(HttpdConnData *connData, char *token, void **arg) 
{
    char buff[128];
    if (token==NULL) return HTTPD_CGI_DONE;

    strcpy(buff, "Unknown");
    if (os_strcmp(token, "spiffs_size") == 0) {
        strncpy(buff, "bloody much", sizeof(buff));
    } else if (os_strcmp(token, "directory") == 0) {
        return spiffs_dir(connData);
    }
    httpdSend(connData, buff, -1);
    return HTTPD_CGI_DONE;
}
#define S_START 0
#define S_WRITE 1
#define S_ERROR 10
#define S_DONE  11
typedef struct _fus {
    int state;
    const char * err;
    spiffs_file fd;
} upload_state_t;

int ICACHE_FLASH_ATTR cgiUploadFile(HttpdConnData *connData) {
    //CgiUploadFlashDef *def=(CgiUploadFlashDef*)connData->cgiArg;
    upload_state_t *state=(upload_state_t *)connData->cgiData;
    char buf[128];

    if (connData->conn == NULL) {
        //Connection aborted. Clean up.
        if (state!=NULL) free(state);
        return HTTPD_CGI_DONE;
    }

    if (state == NULL) {
        //First call. Allocate and initialize state variable.
        printf("File upload cgi start.\n");
        state = malloc(sizeof(upload_state_t));
        if (state == NULL) {
            printf("Can't allocate file upload struct!\n");
            return HTTPD_CGI_DONE;
        }
        memset(state, 0, sizeof(upload_state_t));
        state->state = S_START;
        connData->cgiData = state;
        state->err = "Premature end";

        httpdGetHeader(connData, "X-FileName", buf, 128);
        printf("X-FileName: %s\n", buf);
        printf("Reported file size: %u\n", connData->post->len);

        if (connData->post->len > free_space()) {
            state->err = "Not enough free space";
            state->state = S_ERROR;
        }
        else {
            spiffs_file fd = SPIFFS_open(&fs, buf, SPIFFS_CREAT | 
                    SPIFFS_TRUNC | SPIFFS_WRONLY, 0);
            if (fd < 0) {
                printf("SPIFFS_open errno %d\n", SPIFFS_errno(&fs));
                state->err = "File creation error";
                state->state = S_ERROR;
            } else {
                state->state = S_WRITE;
                state->fd = fd;
            }
        }
    }

    char *data = connData->post->buff;
    int dataLen = connData->post->buffLen;

    if (state->state == S_WRITE && dataLen != 0) {
        if (SPIFFS_write(&fs, state->fd, data, dataLen) < 0) {
            printf("SPIFFS_write errno %d\n", SPIFFS_errno(&fs));
            state->err = "File write error";
            state->state = S_ERROR;
        }
    }

    if (connData->post->len == connData->post->received) {
        SPIFFS_close(&fs, state->fd);

        if (state->state != S_ERROR) {
            state->state = S_DONE;
            state->err = "No error";
        }

        printf("Upload done. Sending response. state=%d\n", state->state);
        httpdStartResponse(connData, state->state == S_ERROR ? 400 : 200);
        httpdHeader(connData, "Content-Type", "text/plain");
        httpdEndHeaders(connData);
        if (state->state != S_DONE) {
            httpdSend(connData, "Firmware image error:", -1);
            httpdSend(connData, state->err, -1);
            httpdSend(connData, "\n", -1);
        } else {
            //httpdSend(connData, "File uploaded ok", -1);
            httpdSend(connData, "Файл успешно закачан", -1);
            httpdSend(connData, "\n", -1);
        }
        free(state);
        return HTTPD_CGI_DONE;
    }

    return HTTPD_CGI_MORE;
}

int ICACHE_FLASH_ATTR delete_by_ids(char * ids) 
{
    spiffs_DIR d;
    struct spiffs_dirent e;
    struct spiffs_dirent *pe = &e;

    printf("Deleting files. Id list: %s\n", ids);
    char * aid;

    while ((aid = strsep(&ids, ";")) != NULL) {
        int id = atoi(aid);
        printf("Looking for file with id=%d...", id);

        SPIFFS_opendir(&fs, "/", &d);
        while ((pe = SPIFFS_readdir(&d, pe))) {
            printf("%d...", pe->obj_id);
            if (pe->obj_id == id) {
                printf("found, removing %s\n", pe->name);
                SPIFFS_remove(&fs, (const char *)pe->name);
                break;
            }
        }
        SPIFFS_closedir(&d);
        printf("next\n");
    }

    return 1;
}

int ICACHE_FLASH_ATTR cgiDelete(HttpdConnData *connData) {
    char buf[256];

    if (connData->conn == NULL) {
        return HTTPD_CGI_DONE;
    }
    httpdGetHeader(connData, "X-SPIFFS-ids", buf, sizeof(buf));
    
    delete_by_ids(buf);

    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type", "text/plain");
    httpdEndHeaders(connData);
    // rien a dire
    return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiSelectBoot(HttpdConnData *connData) {
    char buf[256];

    if (connData->conn == NULL) {
        return HTTPD_CGI_DONE;
    }
    httpdGetHeader(connData, "X-FileName", buf, sizeof(buf));
    
    spiffs_set_boot(buf); 

    httpdStartResponse(connData, 200);
    httpdHeader(connData, "Content-Type", "text/plain");
    httpdEndHeaders(connData);
    // rien a dire
    return HTTPD_CGI_DONE;
}
*/