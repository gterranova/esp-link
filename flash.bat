esptool.py --baud 115200 write_flash --flash_freq 80m --flash_mode qio --flash_size 1MB 0x00000 sdk\ESP8266_NONOS_SDK-version_3.0.4\bin\boot_v1.7.bin 0x01000 firmware\user1.bin 0xFC000 sdk\ESP8266_NONOS_SDK-version_3.0.4\bin\esp_init_data_default_v08.bin 0xFE000 sdk\ESP8266_NONOS_SDK-version_3.0.4\bin\blank.bin