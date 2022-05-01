#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
TARGET = eagle
#FLAVOR = release
FLAVOR = debug

#EXTRA_CCFLAGS += -u

ifndef PDIR # {
GEN_IMAGES= eagle.app.v6.out
GEN_BINS= eagle.app.v6.bin
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
SUBDIRS=    \
	core    \
	
endif # } PDIR
MODULES ?= espfs alexa
#mqtt rest socket webserver syslog
APPDIR = .

CCFLAGS += -Os

TARGET_LDFLAGS = -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static -Wl,--gc-sections

ifeq ($(FLAVOR),debug)
    TARGET_LDFLAGS += -g -O2
endif

ifeq ($(FLAVOR),release)
    TARGET_LDFLAGS += -g -Os
endif

TOP_DIR:=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CUSTOM_LD = $(TOP_DIR)/libs/$(notdir $(LD_FILE))

COMPONENTS_eagle.app.v6 = 

DEPENDS_eagle.app.v6 = \
	$(CUSTOM_LD) \
	$(WEBPAGES)

ifndef PDIR # {
ifneq (,$(findstring espfs,$(MODULES)))
SUBDIRS += modules/espfs 
COMPONENTS_eagle.app.v6 += modules/espfs/libespfs.a 
DEP_LIBS_eagle.app.v6 += modules/espfs/libespfs.a $(WEBPAGES)
endif

ifneq (,$(findstring spiffs,$(MODULES)))
SUBDIRS += modules/spiffs 
COMPONENTS_eagle.app.v6 += modules/spiffs/libspiffs.a 
DEP_LIBS_eagle.app.v6 += modules/spiffs/libspiffs.a 
endif

ifneq (,$(findstring alexa,$(MODULES)))
SUBDIRS += modules/alexa 
COMPONENTS_eagle.app.v6 += modules/alexa/libalexa.a
DEP_LIBS_eagle.app.v6 += modules/alexa/libalexa.a
endif

ifneq (,$(findstring mqtt,$(MODULES)))
SUBDIRS += modules/mqtt 
COMPONENTS_eagle.app.v6 += modules/mqtt/libmqtt.a
DEP_LIBS_eagle.app.v6 += modules/mqtt/libmqtt.a
endif

ifneq (,$(findstring rest,$(MODULES)))
SUBDIRS += modules/rest 
COMPONENTS_eagle.app.v6 += modules/rest/librest.a
DEP_LIBS_eagle.app.v6 += modules/rest/librest.a
endif

ifneq (,$(findstring socket,$(MODULES)))
SUBDIRS += modules/socket 
COMPONENTS_eagle.app.v6 += modules/socket/libsocket.a
DEP_LIBS_eagle.app.v6 += modules/socket/libsocket.a
endif

ifneq (,$(findstring syslog,$(MODULES)))
SUBDIRS += modules/syslog 
COMPONENTS_eagle.app.v6 += modules/syslog/libsyslog.a
DEP_LIBS_eagle.app.v6 += modules/syslog/libsyslog.a
endif

ifneq (,$(findstring webserver,$(MODULES)))
SUBDIRS += modules/webserver 
COMPONENTS_eagle.app.v6 += modules/webserver/libwebserver.a
DEP_LIBS_eagle.app.v6 += modules/webserver/libwebserver.a
endif
endif

COMPONENTS_eagle.app.v6 += \
	core/libesplink.a \

# libraries used in this project, mainly provided by the SDK
LIBS = c gcc hal phy pp net80211 wpa main lwip_536 crypto
ifneq (,$(findstring espfs,$(MODULES)))
	LIBS += webpages
endif
LIBS		:= $(addprefix -l,$(LIBS))

LINKFLAGS_eagle.app.v6 = \
	-L../lib        \
    -T$(CUSTOM_LD)   \
	$(TARGET_LDFLAGS) \
	-Wl,--start-group					\
	$(LIBS) \
	$(DEP_LIBS_eagle.app.v6)	\
	-Wl,--end-group

#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

#UNIVERSAL_TARGET_DEFINES =		\

# Other potential configuration flags include:
#	-DTXRX_TXBUF_DEBUG
#	-DTXRX_RXBUF_DEBUG
#	-DWLAN_CONFIG_CCX
UC = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')
CONFIGURATION_DEFINES =	$(addprefix -D,$(call UC,$(MODULES)))

DEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)

EXTRA_CCFLAGS +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)


#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile

.PHONY: FORCE
FORCE:

