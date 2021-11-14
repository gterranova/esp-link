// Copyright 2015 by Thorsten von Eicken, see LICENSE.txt
#define ALEXA
#define CGIALEXA_DBG

static char *alexa_states[] = {
  "disconnected", "reconnecting", "connecting", "connected", "disabled"
};

#if !defined(ALEXA)
char *alexaState(void) {
  return alexa_states[4];
}
#else
#include <esp8266.h>
#include "cgi.h"
#include "config.h"
#include "alexa.h"
#include "cgialexa.h"

#ifdef CGIALEXA_DBG
#define DBG(format, ...) do { os_printf(format, ## __VA_ARGS__); } while(0)
#else
#define DBG(format, ...) do { } while(0)
#endif

char *alexaState(void) {
  //return alexa_states[mqttClient.connState];
  return alexa_states[4];
}

// Cgi to return MQTT settings
int ICACHE_FLASH_ATTR cgiAlexaGet(HttpdConnData *connData) {
  char buff[1024];
  int len, i;

  if (connData->conn==NULL) return HTTPD_CGI_DONE;

  len = os_sprintf(buff, "{ "
      "\"alexa-enable\":%d, "
      "\"wemo-emulation\":%s, ",
      flashConfig.alexa_enable, flashConfig.wemo_emulation? "true": "false");

  for (i=0; i<flashConfig.alexa_devices; i++) {
    len += os_sprintf(buff+len, 
      "\"d%d-state\": %s, "
      "\"d%d-bri\": %d, ",
      i, devices[i].state? "true": "false",
      i, devices[i].bri);
  }

  len += os_sprintf(buff+len, 
      "\"alexa-devices\":%d }", flashConfig.alexa_devices);

  jsonHeader(connData, 200);
  httpdSend(connData, buff, len);
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiAlexaSet(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE;

  uint8_t isDevicesForm;
  uint8_t i;

  if (getBoolArg(connData, "set-devices", &isDevicesForm) && isDevicesForm) {
    for (i=0; i<flashConfig.alexa_devices; i++) {
      char deviceKey[10];
      uint8_t newState;
      uint16_t newValue;
      os_sprintf(deviceKey, "d%d-state", i);      
      if (getBoolArg(connData, deviceKey, &newState) > 0) {
        devices[i].state = newState;
      } else {
        errorResponse(connData, 400, "Invalid state");
        return HTTPD_CGI_DONE;
      }
      os_sprintf(deviceKey, "d%d-bri", i);      
      if (getUInt16Arg(connData, deviceKey, &newValue) > 0) {
        devices[i].bri = newValue;
      } else {
        errorResponse(connData, 400, "Invalid value");
        return HTTPD_CGI_DONE;
      }
    }
    for (i=0; i<flashConfig.alexa_devices; i++) {
      ExecuteCommandPower(i+1, devices[i].state, devices[i].bri);
    }
    customHttpdStartResponse(connData, 200, "application/json", 2);
    httpdEndHeaders(connData);
    httpdSend(connData, "{}", 2);
    return HTTPD_CGI_DONE;
  };
  
  // handle MQTT server settings
  int8_t alexa_enabled = flashConfig.alexa_enable; // accumulator for changes/errors

  int8_t alexa_en_chg = getBoolArg(connData, "alexa-enable",
      &flashConfig.alexa_enable);

  int8_t alexa_settings_changed = (alexa_enabled != alexa_en_chg);

  char buff[16];

  // handle wemo/hue emulation
  if (getBoolArg(connData, "wemo-emulation", &flashConfig.wemo_emulation)) {
    flashConfig.alexa_devices = 1;
    alexa_settings_changed |= 1;
  }

  // handle alexa devices
  if (!flashConfig.wemo_emulation && httpdFindArg(connData->getArgs, "alexa-devices", buff, sizeof(buff)) > 0) {
    int32_t devices = atoi(buff);
    if (devices > 0 && devices < 9) {
      flashConfig.alexa_devices = devices;
      alexa_settings_changed |= 1;
    } else {
      errorResponse(connData, 400, "Invalid number of devices");
      return HTTPD_CGI_DONE;
    }
  }

  // if server setting changed, we need to "make it so"
  if (alexa_settings_changed) {
    DBG("Alexa settings changed, enable=%d, devices=%d, wemo_emulation=%d\n", flashConfig.alexa_enable, flashConfig.alexa_devices, flashConfig.wemo_emulation);
    if (flashConfig.alexa_enable) {
      //MQTT_Free(&mqttClient); // safe even if not connected
      alexa_init();
      //MQTT_Reconnect(&mqttClient);
    } else {
      //MQTT_Disconnect(&mqttClient);
      //MQTT_Free(&mqttClient); // safe even if not connected
    }
  }

  DBG("Saving config\n");

  if (configSave()) {
    customHttpdStartResponse(connData, 200, "application/json", 2);
    httpdEndHeaders(connData);
    httpdSend(connData, "{}", 2);
  } else {
    httpdStartResponse(connData, 500);
    httpdEndHeaders(connData);
    httpdSend(connData, "Failed to save config", -1);
  }
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiAlexa(HttpdConnData *connData) {
  if (connData->requestType == HTTPD_METHOD_GET) {
    return cgiAlexaGet(connData);
  } else if (connData->requestType == HTTPD_METHOD_POST) {
    return cgiAlexaSet(connData);
  } else {
    jsonHeader(connData, 404);
    return HTTPD_CGI_DONE;
  }
}
#endif
