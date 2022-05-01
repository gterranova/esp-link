#ifndef _USER_CONFIG_H_
#define _USER_CONFIG_H_
#include <c_types.h>
#include <user_interface.h>
#ifdef __WIN32__
#include <_mingw.h>
#endif

#undef SHOW_HEAP_USE
#define DEBUGIP
#define SDK_DBG

#define CMD_DBG
#undef ESPFS_DBG
#undef CGI_DBG
#define CGIFLASH_DBG
#define CGIMQTT_DBG
#define CGIPINS_DBG
#define CGIWIFI_DBG
#define CONFIG_DBG
#define LOG_DBG
#define STATUS_DBG
#define HTTPD_DBG
#define MQTT_DBG
#define MQTTCMD_DBG
#define MQTTCLIENT_DBG
#undef PKTBUF_DBG
#define REST_DBG
#define RESTCMD_DBG
#define SERBR_DBG
#define SERLED_DBG
#define SLIP_DBG
#define UART_DBG
#define MDNS_DBG
#define OPTIBOOT_DBG
#undef SYSLOG_DBG
#define CGISERVICES_DBG

#ifndef SPI_FLASH_SIZE_MAP
#define SPI_FLASH_SIZE_MAP 4 // FLASH_SIZE_32M_MAP_512_512
#endif

#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0xfd000
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#elif (SPI_FLASH_SIZE_MAP == 4)
//#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_SIZE							0x7C000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#else
#error "The flash map is not supported"
#endif

// If defined, the default hostname for DHCP will include the chip ID to make it unique
#undef CHIP_IN_HOSTNAME

extern char* esp_link_version;
extern uint8_t UTILS_StrToIP(const char* str, void *ip);

#endif
