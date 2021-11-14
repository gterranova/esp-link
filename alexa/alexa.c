// Copyright 2015-2016 Doctors of Intelligence & Technology (Shenzhen) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "esp8266.h"
#include "alexa.h"
#include "templates.h"

#include "cgi.h"
#include "httpdespfs.h"
#include "c_types.h"
#include "ip_addr.h"

#include <string.h>

#include "serled.h"
#include "uart.h"
#include "user_config.h"
#include "espconn.h"
#include "config.h"

#ifdef ALEXA_DBG
#define DBG(format, ...) os_printf(format, ## __VA_ARGS__)
#else
#define DBG(format, ...) do { } while(0)
#endif

//static uint8_t alexa_task = 0;
#define MAX_SAVED 512

#define RECV_BUFFSIZE		1024
#define MAX_HTTP_CLIENT_CONN 	1

#define POWER_TOGGLE 2
#define POWER_ON 1
#define POWER_OFF 0

static os_timer_t test_timer;
//static struct espconn user_udp_espconn;

uint8 _deviceId = 1;

//static char text[RECV_BUFFSIZE + 1] = {0};
//static char udp_buff[RECV_BUFFSIZE + 1] = {0};

/*socket id*/
//static int sock_id = -1;

//static char *device_name = "esp";
//static char *persistent_uuid = "Socket-1_0-201612K79DCE000000000";

uint32 mac24; //bottom 24 bits of mac

#define DISCOVER_PORT       1900

static struct espconn discoverConn;
static esp_udp discoverUdp;

static void ICACHE_FLASH_ATTR sentCallback(void *arg)
{
    struct espconn *conn = (struct espconn *)arg;
    int *pDone = (int *)conn->reverse;
    *pDone = 1;
//    os_printf("Send completed\n");
}

static void ICACHE_FLASH_ATTR send_udp_response(void *arg) 
{
    struct espconn *conn = (struct espconn *)arg;
    remot_info *remoteInfo;
    struct espconn responseConn;
    esp_udp responseUdp;
    volatile int done = 0;
    int retries = 100;
    int len;

    struct ip_info info;
    uint8 macAddr[6];

    //os_printf("M_SEARCH resp: %d\n", _deviceId);

    // insert a random delay to avoid udp packet collisions from multiple modules
    os_delay_us(100000 + (unsigned)os_random() % 500000);

    if (espconn_get_connection_info(conn, &remoteInfo, 0) != 0) {
        os_printf("DISCOVER: espconn_get_connection_info failed\n");
        return;
    }

    os_memset(&responseConn, 0, sizeof(responseConn));
    responseConn.type = ESPCONN_UDP;
    responseConn.state = ESPCONN_NONE;
    responseConn.proto.udp = &responseUdp;
    os_memcpy(responseConn.proto.udp->remote_ip, remoteInfo->remote_ip, 4);
    responseConn.proto.udp->remote_port = remoteInfo->remote_port;

    if (wifi_get_ip_info(STATION_IF, &info))
    	wifi_get_macaddr(STATION_IF, macAddr);
    else if (wifi_get_ip_info(SOFTAP_IF, &info))
    	wifi_get_macaddr(SOFTAP_IF, macAddr);
    else
    	memset(macAddr, 0, sizeof(macAddr));

    char macStr[12];
    os_sprintf(macStr, "%02x%02x%02x%02x%02x%02x",
        macAddr[0], macAddr[1], macAddr[2],
        macAddr[3], macAddr[4], macAddr[5]);
    //mac24 = strtol(macStr, 0, 16);

    if (espconn_create(&responseConn) == 0) {
        char response[1024];
        os_bzero(response, sizeof(response));

        if (flashConfig.wemo_emulation) {
            char serial[20];

            char index[8] = { 0 };

            if (_deviceId > 1) {  // Keep backward compatibility
                os_sprintf(index, "%02x", _deviceId);
            }
            os_sprintf(serial, "201612K%08X%s", system_get_chip_id(), index);
            
            char uuid[32];

            os_sprintf(uuid, "Socket-1_0-%s", serial);		
                            
            //volatile int done = 0;
            //int retries = 100;
            len = os_snprintf(response, sizeof(response), 
                WEMO_UDP_RESPONSE_TEMPLATE, IP2STR(&conn->proto.udp->local_ip), uuid);

            //os_printf("%s\n", buf);

        } else {
            len = os_snprintf(response, sizeof(response), 
                FAUXMO_UDP_RESPONSE_TEMPLATE, 
                IP2STR(&conn->proto.udp->local_ip),
                80,
                MAC2STR(macAddr),
                MAC2STR(macAddr)
            );
        }
        espconn_regist_sentcb(&responseConn, sentCallback);
        responseConn.reverse = (int *)&done;
        espconn_send(&responseConn, (uint8 *)response, len);
    //  os_printf("Waiting for send to complete\n");
        while (!done && --retries >= 0)
            os_delay_us(10000);
        if (!done)
            os_printf("DISCOVER: failed to send discovery response\n");
    //        os_printf("Done waiting for send to complete\n");
        espconn_delete(&responseConn);
    }        

}

int ICACHE_FLASH_ATTR cgiHandleUpnpEvent(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE; // Connection aborted
  char buff[512];

  os_memset(buff, 0, sizeof(buff));

  // iterate through the data received and program the AVR one block at a time
  HttpdPostData *post = connData->post;
  char *saved = &buff[0];
  while (post->buffLen > 0) {
    // first fill-up the saved buffer
    short saveLen = strlen(saved);
    if (saveLen < MAX_SAVED) {
      short cpy = MAX_SAVED-saveLen;
      if (cpy > post->buffLen) cpy = post->buffLen;
      os_memcpy(saved+saveLen, post->buff, cpy);
      saveLen += cpy;
      saved[saveLen] = 0; // string terminator
      os_memmove(post->buff, post->buff+cpy, post->buffLen-cpy);
      post->buffLen -= cpy;
      //DBG("OB cp %d buff->saved\n", cpy);
    }
  }
  //os_printf("BUFFER: %s", buff);

  //differentiate get and set state
  char state = 'G';
  if (connData->requestType == HTTPD_METHOD_POST && (os_strstr(buff, "SetBinaryState") != NULL)) {
    state = 'S';
    uint8_t power = POWER_TOGGLE;
    if (os_strstr(buff, "State>1</Binary") != NULL) {
      power = POWER_ON;
	  devices[0].state = POWER_ON;
    }
    else if (os_strstr(buff, "State>0</Binary") != NULL) {
      power = POWER_OFF;
	  devices[0].state = POWER_OFF;
    }
    if (power == POWER_TOGGLE) {
	  devices[0].state = devices[0].state == POWER_ON? POWER_OFF: POWER_ON;
	}
    ExecuteCommandPower(1, devices[0].state, 0);

  }
  //os_printf("BUFFER: ");
  //os_printf(state=='G'? "GETPW": "SETPW");
  //os_printf(device_on? " ON\n": " OFF\n");

  os_sprintf(buff, WEMO_UPNP_RESPONSE_TEMPLATE, state, devices[0].state, state);

  httpdStartResponse(connData, 200);
  httpdHeader(connData, "Content-Type", "text/xml");
  httpdHeader(connData, "Connection", "close");
  httpdEndHeaders(connData);
  httpdSend(connData, buff, os_strlen(buff));
  return HTTPD_CGI_DONE;
}

static void ICACHE_FLASH_ATTR discoverRecvCallback(void *arg, char *data, unsigned short len)
{
    //uint32_t *rxNext;
    //uint32_t myAddress = (conn->proto.udp->local_ip[3] << 24)
    //                   | (conn->proto.udp->local_ip[2] << 16)
    //                   | (conn->proto.udp->local_ip[1] << 8)
    //                   |  conn->proto.udp->local_ip[0];
    if (data == NULL) 
        return;

  	//os_printf("recv udp data: %s\n", data);
 
    if (os_strstr(data, "M-SEARCH") != NULL) 
    {
        if(os_strstr(data, "upnp:rootdevice") != NULL || os_strstr(data, "device:basic:1") > 0) {
            send_udp_response(arg);
        }        
	}
}

void ICACHE_FLASH_ATTR initDiscovery(void)
{
    struct ip_info ipconfig;
	struct ip_info uPnP_MulticastIP;

   //disarm timer first
    os_timer_disarm(&test_timer);
    
   //get ip info of ESP8266 station
    wifi_get_ip_info(STATION_IF, &ipconfig);

    if (wifi_station_get_connect_status() == STATION_GOT_IP && ipconfig.ip.addr != 0)
   {
      //os_printf("got ip !!!\n");
      wifi_set_opmode(STATION_MODE); 
      wifi_set_broadcast_if(STATION_MODE);

    discoverConn.type = ESPCONN_UDP;
    discoverConn.state = ESPCONN_NONE;
    discoverUdp.local_port = DISCOVER_PORT;
    discoverUdp.remote_ip[0] = 239;
    discoverUdp.remote_ip[1] = 255;
    discoverUdp.remote_ip[2] = 255;
    discoverUdp.remote_ip[3] = 250;	
    discoverConn.proto.udp = &discoverUdp;
    
    if (espconn_create(&discoverConn) != 0) {
        os_printf("DISCOVER: espconn_create failed\n");
            os_timer_setfn(&test_timer, (os_timer_func_t *)initDiscovery, NULL);
            os_timer_arm(&test_timer, 100, 0);
        return;
    }

    if (espconn_regist_recvcb(&discoverConn, discoverRecvCallback) != 0) {
        os_printf("DISCOVER: espconn_regist_recvcb failed\n");
            os_timer_setfn(&test_timer, (os_timer_func_t *)initDiscovery, NULL);
            os_timer_arm(&test_timer, 100, 0);
        return;
    }

    IP4_ADDR(&uPnP_MulticastIP.ip, 239, 255, 255, 250);
    if (espconn_igmp_join(&ipconfig.ip, &uPnP_MulticastIP.ip) != 0) {
        os_printf("DISCOVER: espconn_igmp_join failed\n");
            os_timer_setfn(&test_timer, (os_timer_func_t *)initDiscovery, NULL);
            os_timer_arm(&test_timer, 100, 0);
        return;
    }

    os_printf("DISCOVER: initialized\n");
    }
   else
   {
        if ((wifi_station_get_connect_status() == STATION_WRONG_PASSWORD ||
                wifi_station_get_connect_status() == STATION_NO_AP_FOUND ||
                wifi_station_get_connect_status() == STATION_CONNECT_FAIL))
        {
         os_printf("connect fail !!! \r\n");
        }
      else
      {
           //re-arm timer to check ip
            os_timer_setfn(&test_timer, (os_timer_func_t *)initDiscovery, NULL);
            os_timer_arm(&test_timer, 100, 0);
        }
    }  
}

int ICACHE_FLASH_ATTR tpl_hue_description(HttpdConnData *connData, char *token, void **arg)
{
	TplData *tpd=connData->cgiData;
    char buf[128];
    if (!token) return HTTPD_CGI_DONE;
	
	//os_printf("URL: %s (%d)\n", connData->url, tpd->count);
	
	char serial[20];

	char index[8] = { 0 };
	uint8 _deviceId = tpd->count;
	if (_deviceId > 1) {  // Keep backward compatibility
		os_sprintf(index, "%02x", _deviceId);
	}
	os_sprintf(serial, "201612K%08X%s", system_get_chip_id(), index);
	
	char uuid[32];

	os_sprintf(uuid, "Socket-1_0-%s", serial);		

    os_bzero(buf, sizeof(buf));

    if (os_strcmp(token, "SerialNumber") == 0) {
        os_sprintf(buf, "%s", serial);
    }
    else if (os_strcmp(token, "DeviceId") == 0) {
        os_sprintf(buf, "%d", tpd->count);
    }
    else if (os_strcmp(token, "UUID") == 0) {
        os_sprintf(buf, "%s", uuid);
    }
    else if (os_strcmp(token, "FriendlyName") == 0) {
        os_sprintf(buf, "%s", "esplink");
    }
    else if (os_strcmp(token, "IpAddress") == 0) {
        struct ip_info info;
        wifi_get_ip_info(STATION_IF, &info);
        int wifi_status = wifi_station_get_connect_status();
        if (wifi_status == STATION_GOT_IP && info.ip.addr != 0)
            os_sprintf(buf, IPSTR, IP2STR(&info.ip));
        else if (wifi_get_ip_info(SOFTAP_IF, &info))
            os_sprintf(buf, IPSTR, IP2STR(&info.ip));
    }
    else if (os_strcmp(token, "MacAddress") == 0) {
        uint8 macaddr[6];
        if (wifi_get_macaddr(STATION_IF, macaddr)) {
            os_sprintf(buf, "%02x%02x%02x%02x%02x%02x\n",
                macaddr[0], macaddr[1], macaddr[2],
                macaddr[3], macaddr[4], macaddr[5]);
        }
    }

    httpdSend(connData, buf, -1);
    return HTTPD_CGI_DONE;
}

// print various Wifi information into json buffer
int ICACHE_FLASH_ATTR printDeviceInfo(char *buff, uint8 id, bool shortVersion) {
    struct ip_info info;
    uint8 macAddr[6];

    if (wifi_get_ip_info(STATION_IF, &info))
    	wifi_get_macaddr(STATION_IF, macAddr);
    else if (wifi_get_ip_info(SOFTAP_IF, &info))
    	wifi_get_macaddr(SOFTAP_IF, macAddr);
    else
    	memset(macAddr, 0, sizeof(macAddr));

    int len;
    if (!shortVersion) {
        len = os_sprintf(buff,
            FAUXMO_DEVICE_JSON_TEMPLATE,
            devices[id].name, MAC2STR(macAddr), id+1,
            devices[id].state ? "true": "false"
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)                
            ,devices[id].bri
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
            ,devices[id].hue,
            devices[id].sat,
            devices[id].ct,
            (devices[id].hue != 0 || devices[id].sat !=0) ? "hs": "ct"
#endif
#endif
            );

    } else {
        len = os_sprintf(buff,
            FAUXMO_DEVICE_JSON_TEMPLATE_SHORT,
            devices[id].name, MAC2STR(macAddr), id+1,
            devices[id].state ? "true": "false"
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)                
            ,devices[id].bri
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
            ,devices[id].hue,
            devices[id].sat,
            devices[id].ct,
            devices[id].ct? "ct": "hs"
#endif
#endif
        );

    }
    return len;
}

uint32 encodeLightKey(uint32 idx)
{
    struct ip_info info;
    uint8 macAddr[6];

    if (wifi_get_ip_info(STATION_IF, &info))
    	wifi_get_macaddr(STATION_IF, macAddr);
    else if (wifi_get_ip_info(SOFTAP_IF, &info))
    	wifi_get_macaddr(SOFTAP_IF, macAddr);
    else
    	memset(macAddr, 0, sizeof(macAddr));

    char macStr[6];
        os_sprintf(macStr, "%02x%02x%02x",
            macAddr[3], macAddr[4], macAddr[5]);
    mac24 = strtol(macStr, 0, 16);

    return (mac24<<7) | idx;
}

// get device index from Json key
uint32 decodeLightKey(uint32 key)
{
    struct ip_info info;
    uint8 macAddr[6];

    if (wifi_get_ip_info(STATION_IF, &info))
    	wifi_get_macaddr(STATION_IF, macAddr);
    else if (wifi_get_ip_info(SOFTAP_IF, &info))
    	wifi_get_macaddr(SOFTAP_IF, macAddr);
    else
    	memset(macAddr, 0, sizeof(macAddr));

    char macStr[6];
        os_sprintf(macStr, "%02x%02x%02x",
            macAddr[3], macAddr[4], macAddr[5]);
    mac24 = strtol(macStr, 0, 16);
    return (((uint32)key>>7) == mac24) ? (key & 127U) : 255U;
}

void ICACHE_FLASH_ATTR customHttpdStartResponse(HttpdConnData *conn, int code, char* content_type, int len) {
  char buff[128];
  int l;
  //conn->priv->code = code;
  l = os_sprintf(buff, FAUXMO_TCP_HEADERS, code, code==200? "OK": "ERROR", content_type, len);
  httpdSend(conn, buff, l);
}

int ICACHE_FLASH_ATTR handleListEvent(HttpdConnData *connData) {
  char response[4096];
  uint16 len;

  char systemChipId[8];
  os_snprintf(systemChipId, sizeof(systemChipId), "%06x", system_get_chip_id());

  char *lights = os_strstr(connData->url, "lights");
  if (lights == NULL) {
    if (os_strstr(connData->url, systemChipId) == NULL) {
        int len = os_snprintf(response, sizeof(response), "[{\"success\":{\"username\": \"%s\"}}]", systemChipId);
        customHttpdStartResponse(connData, 200, "application/json", len);
        httpdEndHeaders(connData);
        httpdSend(connData, response, len);
        return HTTPD_CGI_DONE;
    }
  }

  if (lights == NULL) {
    uint8 macAddr[6];
    struct ip_info info;

    if (wifi_get_ip_info(STATION_IF, &info))
    	wifi_get_macaddr(STATION_IF, macAddr);
    else if (wifi_get_ip_info(SOFTAP_IF, &info))
    	wifi_get_macaddr(SOFTAP_IF, macAddr);
    else
    	memset(macAddr, 0, sizeof(macAddr));

    len = os_sprintf(response, "{\"lights\":{");
    for (uint32 i=0; i< flashConfig.alexa_devices; i++) {
        if (i>0) len += os_sprintf(response+len, ",");
        len += os_sprintf(response+len, "\"%d\":", encodeLightKey(i+1));
        len += printDeviceInfo(response+len, i, true);
    }
    len += os_sprintf(response+len, "},\"groups\":{},\"schedules\":{},\"config\":{\"name\":\"Philips hue\","
    "\"mac\":\"" MACSTR "\",\"dhcp\":true,\"ipaddress\":\"" IPSTR "\",\"netmask\":\"" IPSTR "\","
    "\"gateway\":\"" IPSTR "\",\"proxyaddress\":\"none\",\"proxyport\":0,\"bridgeid\":\"%02X%02X%02XFFFE%02X%02X%02X\","
    "\"UTC\":\"2021-09-12T20:25:26\",\"whitelist\":{\"%s\":{\"last use date\":\"2021-09-12T20:25:26\",\"create date\":\"2021-09-12T20:25:26\","
    "\"name\":\"Remote\"}},\"swversion\":\"01041302\",\"apiversion\":\"1.17.0\",\"swupdate\":{\"updatestate\":0,\"url\":\"\",\"text\":\"\",\"notify\": false},\"linkbutton\":false,\"portalservices\":false}}",
    MAC2STR(macAddr), 
    IP2STR(&info.ip),
    IP2STR(&info.netmask),
    IP2STR(&info.gw),
    MAC2STR(macAddr), 
    systemChipId
    );  
    customHttpdStartResponse(connData, 200, "application/json", len);
    httpdEndHeaders(connData);
    httpdSend(connData, response, len);
    return HTTPD_CGI_DONE;
  }

  uint32 id = atoi(lights+7);

  //os_printf("Asking for light #%d\n", id);

  // Client is requesting all devices
  if (0 == id) {
    len = os_sprintf(response, "{");
    for (uint32 i=0; i< flashConfig.alexa_devices; i++) {
        if (i>0) len += os_sprintf(response+len, ",");
        len += os_sprintf(response+len, "\"%d\":", encodeLightKey(i+1));
        len += printDeviceInfo(response+len, i, false);
    }
    len += os_sprintf(response+len, "}");

  // Client is requesting a single device
  } else {
    uint32 decId = decodeLightKey(id);
    if (os_strstr(connData->url, "state") != NULL) {
        len = os_snprintf(
            response, sizeof(response),
            FAUXMO_TCP_STATE_TEMPLATE,
            devices[decId-1].state ? "true" : "false"
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)             
            ,devices[decId-1].bri
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
            ,devices[decId-1].hue,
            devices[decId-1].sat,
            devices[decId-1].ct
#endif
#endif
        );
    } else {
        len = printDeviceInfo(response, decId-1, false);
    }
  }

  //os_printf("RESPONSE: %s", response);

  customHttpdStartResponse(connData, 200, "application/json", len);
  httpdEndHeaders(connData);
  httpdSend(connData, response, os_strlen(response));
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR handleControlEvent(HttpdConnData *connData) {
    HttpdPostData *post = connData->post;

    if (connData->conn==NULL || post->buffLen == 0) return HTTPD_CGI_DONE; // Connection aborted
    char buff[post->buffLen+1];

    os_memset(buff, 0, sizeof(buff));

    // iterate through the data received and program the AVR one block at a time
    char *saved = &buff[0];
    while (post->buffLen > 0) {
        // first fill-up the saved buffer
        short saveLen = strlen(saved);
        if (saveLen < MAX_SAVED) {
        short cpy = MAX_SAVED-saveLen;
        if (cpy > post->buffLen) cpy = post->buffLen;
        os_memcpy(saved+saveLen, post->buff, cpy);
        saveLen += cpy;
        saved[saveLen] = 0; // string terminator
        os_memmove(post->buff, post->buff+cpy, post->buffLen-cpy);
        post->buffLen -= cpy;
        //DBG("OB cp %d buff->saved\n", cpy);
        }
    }
 
    if (os_strstr(buff, "devicetype") != NULL) {
        char response[100];
        int len = os_snprintf(response, sizeof(response), "[{\"success\":{\"username\": \"%06x\"}}]", system_get_chip_id());
        customHttpdStartResponse(connData, 200, "application/json", len);
        httpdEndHeaders(connData);
        httpdSend(connData, response, len);
        return HTTPD_CGI_DONE;
    }

    if (os_strstr(connData->url, "state") != NULL && os_strlen(buff) > 0) {
        char *pos = os_strstr(connData->url, "lights");
        if (pos == NULL) return HTTPD_CGI_DONE; // not found, skip
        os_printf("%s \r\n", buff);
        int id = atoi(pos+7);
        if (id > 0) {
            uint32 decId = decodeLightKey(id);
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)                
            // Brightness
			pos = os_strstr(buff, "bri");
			if (pos != NULL) {
				unsigned char value = atoi(pos+5);
                devices[decId-1].bri = value;
                //devices[decId-1].state = (value > 4);
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
			} else {
                pos = os_strstr(buff, "ct");
                if (pos != NULL) {
                    uint16_t value = atoi(pos+4);
                    devices[decId-1].ct = value;
                    devices[decId-1].hue = 0;
                    devices[decId-1].sat = 0;
                    //devices[decId-1].state = (value > 4);
                } else {
                    pos = os_strstr(buff, "hue");
                    if (pos != NULL) {
                        uint32_t value = atoi(pos+5);
                        devices[decId-1].hue = value;
                        pos = os_strstr(buff, "sat");
                        if (pos != NULL) {
                            value = atoi(pos+5);
                            devices[decId-1].sat = value;
                        }
                        //devices[decId-1].state = (value > 4);
#endif
                    } else 
#endif
                    
                    if (os_strstr(buff, "false") != NULL) {
                        devices[decId-1].state = false;
                    } else {
                        devices[decId-1].state = true;
                    }
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
                }
            }
#endif

			char response[os_strlen(FAUXMO_TCP_STATE_RESPONSE)+100];
			int len = os_snprintf(
				response, sizeof(response),
				FAUXMO_TCP_STATE_RESPONSE,
				id, devices[decId-1].state ? "true" : "false"
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)                   
                ,id, devices[decId-1].bri
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
                ,id, devices[decId-1].hue,
                id, devices[decId-1].sat,
                id, devices[decId-1].ct
#endif
#endif
			);
            customHttpdStartResponse(connData, 200, "application/json", len);
            httpdEndHeaders(connData);
            httpdSend(connData, response, len);

            // notify mcu
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)                
            ExecuteCommandPower(decId, devices[decId-1].state, devices[decId-1].bri);
#else
            ExecuteCommandPower(decId, devices[decId-1].state, 0);
#endif            
            return HTTPD_CGI_DONE;

			//if (_setCallback) {
			//	_setCallback(id, _devices[decId-1].name, _devices[decId-1].state, _devices[decId-1].bri);
			//}
        }
    }
    
    return HTTPD_CGI_DONE;
}

void ICACHE_FLASH_ATTR ExecuteCommandPower(int device, bool power, uint8_t bri) {
    // notify mcu
    char cmd[255];
    if (bri == 0) {
        os_snprintf(
            cmd, sizeof(cmd),
            "power %d %d\n",
            device, power
        );
    } else {
        os_snprintf(
            cmd, sizeof(cmd),
            "power %d %d %d\n",
            device, power, bri
        );
    }
    serledFlash(50); // short blink on serial LED
    char *str = &cmd[0];
    while(*str)
    {
        uart_tx_one_char(UART0, *str++);
    }
}

int ICACHE_FLASH_ATTR cgiHandleApiEvent(HttpdConnData *connData) {
    if (flashConfig.wemo_emulation) {
        return cgiHandleUpnpEvent(connData);
    } else {
        if (connData->requestType == HTTPD_METHOD_POST || connData->requestType == HTTPD_METHOD_PUT ) {
            return handleControlEvent(connData);
        } 
        else if (connData->requestType == HTTPD_METHOD_GET) {
            return handleListEvent(connData);
        }
        return HTTPD_CGI_DONE;
    }
}

void alexa_init(void)
{
  if (flashConfig.alexa_enable) {
    devices = (fauxmoesp_device_t *)os_malloc(flashConfig.alexa_devices*sizeof(fauxmoesp_device_t));
    if (!devices) {
        os_printf("Malloc failed\n");
        return;
    }
    unsigned int i;
    for (i=0; i<flashConfig.alexa_devices; i++) {
        devices[i].name = os_malloc(9*sizeof(char));
        if (!devices[i].name) {
            os_printf("Malloc name failed\n");
            return;
        }
        os_sprintf(devices[i].name, "esplink%d", i);
        devices[i].state = false;
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)        
        devices[i].bri = 254;
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
        devices[i].hue = 0;
        devices[i].sat = 0;
        devices[i].ct = 199;
#endif
#endif
    }
	initDiscovery();
  }
}
