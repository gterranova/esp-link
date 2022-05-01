#
# Makefile for esp-link - https://github.com/jeelabs/esp-link
#
MODULES = espfs
#mqtt rest webserver

GZIP_COMPRESSION ?= yes
HTML_PATH = $(abspath .)/
WIFI_PATH = $(HTML_PATH)wifi/

# Output directors to store intermediate compiled files
# relative to the project directory
TOP_DIR:=$(abspath ..)

MKESPFSIMAGE_PATH = $(TOP_DIR)/tools/mkespfsimage
BUILD_DIR	= $(abspath ./.output)
OUT_DIR	    = $(TOP_DIR)/libs
DATA_DIR	= $(TOP_DIR)/data

ESPFS_AR = $(OUT_DIR)/libwebpages.a

NODE_APP = $(TOP_DIR)/tools/compresshtml

# select which tools to use as compiler, librarian and linker
AR		:= $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-ar
OBJCP		:= $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-objcopy

ifeq ("$(GZIP_COMPRESSION)","yes")
CFLAGS		+= -DGZIP_COMPRESSION
endif

.PHONY: all

all: $(DATA_DIR) $(ESPFS_AR)

clean:
	$(Q) rm -rf $(BUILD_DIR)
	$(Q) rm -rf $(DATA_DIR)
	$(Q) rm -rf $(ESPFS_AR)

$(BUILD_DIR):
	$(Q) mkdir -p $@

$(DATA_DIR): $(HTML_PATH)/ $(HTML_PATH)/wifi/ 
	$(Q) rm -rf $(DATA_DIR); mkdir $(DATA_DIR); mkdir $(DATA_DIR)/wifi;
	$(Q) cp -r $(HTML_PATH)/*.ico $(DATA_DIR);
	$(Q) cp -r $(HTML_PATH)/*.xml $(DATA_DIR);
	$(Q) cp -r $(HTML_PATH)/wifi/*.png $(DATA_DIR)/wifi;
	$(Q) node $(NODE_APP) "$(HTML_PATH)" "$(DATA_DIR)";
ifeq (,$(findstring mqtt,$(OPTIONAL_MODULES)))
	$(Q) rm -rf $(DATA_DIR)/mqtt.html
	$(Q) rm -rf $(DATA_DIR)/mqtt.js
endif
ifeq (,$(findstring webserver,$(OPTIONAL_MODULES)))
	$(Q) rm -rf $(DATA_DIR)/webserver.html
	$(Q) rm -rf $(DATA_DIR)/userpage.js
endif
ifeq (,$(findstring alexa,$(OPTIONAL_MODULES)))
	$(Q) rm -rf $(DATA_DIR)/alexa.*
	$(Q) rm -rf $(DATA_DIR)/*.xml
endif

$(MKESPFSIMAGE_PATH)/mkespfsimage: $(MKESPFSIMAGE_PATH)/
	$(Q) $(MAKE) -C $(MKESPFSIMAGE_PATH) GZIP_COMPRESSION="$(GZIP_COMPRESSION)"

$(ESPFS_AR): $(DATA_DIR) $(BUILD_DIR) $(MKESPFSIMAGE_PATH)/mkespfsimage
	$(Q) cd $(DATA_DIR); find . \! -name \*- | $(MKESPFSIMAGE_PATH)/mkespfsimage > $(BUILD_DIR)/espfs.img; cd ..;
	$(Q) ls -sl $(BUILD_DIR)/espfs.img
	cd $(BUILD_DIR); $(Q) $(OBJCP) -I binary -O elf32-xtensa-le -B xtensa --rename-section .data=.espfs espfs.img $(BUILD_DIR)/espfs_img.o; cd $(TOP_DIR)
	rm -rf $(BUILD_DIR)/espfs.img
	$(AR) cru $@ $(BUILD_DIR)/espfs_img.o
	rm -rf $(BUILD_DIR)/espfs_img.o

