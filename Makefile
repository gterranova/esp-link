#  copyright (c) 2018 quackmore-ff@yahoo.com
#
.NOTPARALLEL:

# Local project and SDK reference
#TOP_DIR:=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
TOP_DIR:=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CCFLAGS:= -I$(TOP_DIR)/include -I$(TOP_DIR)/src/include -I$(TOP_DIR)/src/modules -I$(SDK_DIR)/include \
        -I$(SDK_DIR)/include -I$(SDK_DIR)/extra_include -I$(SDK_DIR)/driver_lib/include -I$(SDK_DIR)/include/espressif \
		-I$(SDK_DIR)/include/lwip -I$(SDK_DIR)/include/lwip/ipv4 -I$(SDK_DIR)/include/lwip/ipv6 -I$(SDK_DIR)/include/nopoll \
		-I$(SDK_DIR)/include/ssl \
        -I$(SDK_DIR)/include/json \
        -I$(SDK_DIR)/include/openssl 
CXXFLAGS:= -I$(TOP_DIR)/include -I$(TOP_DIR)/src/include -I$(TOP_DIR)/src/modules -I$(SDK_DIR)/include \
        -I$(SDK_DIR)/include -I$(SDK_DIR)/extra_include -I$(SDK_DIR)/driver_lib/include -I$(SDK_DIR)/include/espressif \
		-I$(SDK_DIR)/include/lwip -I$(SDK_DIR)/include/lwip/ipv4 -I$(SDK_DIR)/include/lwip/ipv6 -I$(SDK_DIR)/include/nopoll \
		-I$(SDK_DIR)/include/ssl \
        -I$(SDK_DIR)/include/json \
        -I$(SDK_DIR)/include/openssl 
LDFLAGS:= -L$(TOP_DIR)/libs -L$(SDK_DIR)/lib -L$(SDK_DIR)/ld $(LDFLAGS)

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

-include /esp-link/local.conf

ifdef DEBUG
	CCFLAGS += -ggdb -O0
	LDFLAGS += -ggdb
else
	CCFLAGS += -Os
endif

#############################################################
# Select compile
#
# Base directory for the compiler. Needs a / at the end.
# Typically you'll install https://github.com/pfalcon/esp-open-sdk
# IMPORTANT: use esp-open-sdk `make STANDALONE=n`: the SDK bundled with esp-open-sdk will *not* work!
XTENSA_TOOLS_ROOT ?= $(abspath /xtensa-lx106-elf)

ifeq ($(OS),Windows_NT)
	# WIN32
	# We are under windows.
	ifeq ($(XTENSA_CORE),lx106)
		# It is xcc
		AR = xt-ar
		CC = xt-xcc
		CXX = xt-xcc
		NM = xt-nm
		CPP = xt-cpp
		OBJCOPY = xt-objcopy
        OBJDUMP = xt-objdump
		CCFLAGS += --rename-section .text=.irom0.text --rename-section .literal=.irom0.literal
	else 
		# It is gcc, may be cygwin
		# Can we use -fdata-sections?
		CCFLAGS += -ffunction-sections -fdata-sections
		AR = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-ar
		CC = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-gcc
		CXX = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-g++
		NM = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-nm
		CPP = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-cpp
		OBJCOPY = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-objcopy
		OBJDUMP = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-objdump
	endif
	ifndef COMPORT
		ESPPORT = com1
	else
		ESPPORT = $(COMPORT)
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
	# ->AMD64
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
	# ->IA32
	endif
else
	# We are under other system, may be Linux. Assume using gcc.
	# Can we use -fdata-sections?
	ifndef COMPORT
		ESPPORT = /dev/ttyUSB0
	else
		ESPPORT = $(COMPORT)
	endif
	CCFLAGS += -ffunction-sections -fdata-sections
	AR = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-ar
	CC = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-gcc
	CXX = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-g++
	NM = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-nm
	CPP = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-cpp
	OBJCOPY = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-objcopy
	OBJDUMP = $(XTENSA_TOOLS_ROOT)/bin/xtensa-lx106-elf-objdump

	UNAME_S := $(shell uname -s)
	
	ifeq ($(UNAME_S),Linux)
	# LINUX
	endif
	ifeq ($(UNAME_S),Darwin)
	# OSX
	endif
	
	UNAME_P := $(shell uname -p)
	ifeq ($(UNAME_P),x86_64)
	# ->AMD64
	endif
	ifneq ($(filter %86,$(UNAME_P)),)
	# ->IA32
	endif
	ifneq ($(filter arm%,$(UNAME_P)),)
	# ->ARM
	endif
endif
FIRMWAREDIR = $(TOP_DIR)/firmware
#############################################################
ESPTOOL ?= esptool.py
# espressif tool to concatenate sections for OTA upload using bootloader v1.2+
#APPGEN_TOOL	?= $(SDK_DIR)/tools/gen_appbin.py
APPGEN_TOOL	?= $(TOP_DIR)/tools/gen_appbin_py3.py

CSRCS ?= $(wildcard *.c)
CXXSRCS ?= $(wildcard *.cpp)
ASRCs ?= $(wildcard *.s)
ASRCS ?= $(wildcard *.S)
SUBDIRS ?= $(patsubst %/,%,$(dir $(filter-out tools/Makefile,$(wildcard */Makefile))))

ODIR := .output
OBJODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/obj

OBJS := $(CSRCS:%.c=$(OBJODIR)/%.o) \
        $(CXXSRCS:%.cpp=$(OBJODIR)/%.o) \
        $(ASRCs:%.s=$(OBJODIR)/%.o) \
        $(ASRCS:%.S=$(OBJODIR)/%.o)

DEPS := $(CSRCS:%.c=$(OBJODIR)/%.d) \
        $(CXXSCRS:%.cpp=$(OBJODIR)/%.d) \
        $(ASRCs:%.s=$(OBJODIR)/%.d) \
        $(ASRCS:%.S=$(OBJODIR)/%.d)

LIBODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/lib
OLIBS := $(GEN_LIBS:%=$(LIBODIR)/%)

IMAGEODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/image
OIMAGES := $(GEN_IMAGES:%=$(IMAGEODIR)/%)

BINODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/bin
OBINS := $(GEN_BINS:%=$(BINODIR)/%)

GIT_VERSION := $(shell git --no-pager describe --tags --always --dirty)

#
# Note: 
# https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
# If you add global optimize options like "-O2" here 
# they will override "-Os" defined above.
# "-Os" should be used to reduce code size
#
CCFLAGS += 			\
	-std=c99 -Werror -Wpointer-arith -Wundef -Wall -Wl,-EL -fno-inline-functions \
	-nostdlib -mlongcalls -mtext-section-literals \
	-D__ets__ -DICACHE_FLASH -Wno-address -DFIRMWARE_SIZE=503808 -DMCU_RESET_PIN=12 -DMCU_ISP_PIN=13 \
	-DLED_CONN_PIN=0 -DLED_SERIAL_PIN=14 \
	-DESPFS_GZIP -DSTA_SSID="FASTWEB-FKDH3S" -DSTA_PASS="5P7M3ST99P" -DGZIP_COMPRESSION -DCHANGE_TO_STA \
	-DVERSION="esp-link $(GIT_VERSION)"

CXXFLAGS += 			\
	-c -fno-rtti -fno-exceptions -std=c++11 -Os -Wpointer-arith -Wundef -Wall \
	-Wl,-EL -fno-inline-functions \
	-nostdlib -mlongcalls -mtext-section-literals \
	-ffunction-sections -fdata-sections -Wno-address \
	-DESP8266 -DARDUINO_ARCH_ESP8266 -DARDUINO_ESP8266_ESP12 \
	-D__ets__ -DICACHE_FLASH -Wno-address -DFIRMWARE_SIZE=503808 -DMCU_RESET_PIN=12 -DMCU_ISP_PIN=13 \
	-DLED_CONN_PIN=0 -DLED_SERIAL_PIN=14 \
	-DESPFS_GZIP -DSTA_SSID="FASTWEB-FKDH3S" -DSTA_PASS="5P7M3ST99P" -DGZIP_COMPRESSION -DCHANGE_TO_STA \
	-DVERSION="esp-link $(GIT_VERSION)"

CFLAGS = $(CCFLAGS) $(DEFINES) $(EXTRA_CCFLAGS) $(STD_CFLAGS) $(INCLUDES)
CXFLAGS = $(CXXFLAGS) $(DEFINES) $(EXTRA_CXXFLAGS) $(STD_CFLAGS) $(INCLUDES)
DFLAGS = $(CCFLAGS) $(DDEFINES) $(EXTRA_CCFLAGS) $(STD_CFLAGS) $(INCLUDES)


#############################################################
# Functions
#

define ShortcutRule
$(1): .subdirs $(2)/$(1)
endef

define MakeLibrary
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(LIBODIR)/$(1).a: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	@mkdir -p $$(LIBODIR)
	$$(if $$(filter %.a,$$?),mkdir -p $$(EXTRACT_DIR)_$(1))
	$$(if $$(filter %.a,$$?),cd $$(EXTRACT_DIR)_$(1); $$(foreach lib,$$(filter %.a,$$?),$$(AR) xo $$(UP_EXTRACT_DIR)/$$(lib);))
	$$(vecho) "AR $(1).a"
	$$(Q) $$(AR) cru $$@ $$(filter %.o,$$?) $$(if $$(filter %.a,$$?),$$(EXTRACT_DIR)_$(1)/*.o)
	$$(if $$(filter %.a,$$?),$$(RM) -r $$(EXTRACT_DIR)_$(1))
endef

define MakeImage
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(IMAGEODIR)/$(1).out: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	@mkdir -p $$(IMAGEODIR)
	$$(CC) $$(LDFLAGS) $$(if $$(LINKFLAGS_$(1)),$$(LINKFLAGS_$(1)),$$(LINKFLAGS_DEFAULT) $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1))) -o $$@ 
endef

$(BINODIR)/%.bin: $(IMAGEODIR)/%.out
	@mkdir -p $(BINODIR)
	
ifeq ($(APP), 0)
	@$(RM) -r $(BINODIR)/eagle.S $(BINODIR)/eagle.dump
	@$(OBJDUMP) -x -s $< > $(BINODIR)/eagle.dump
	@$(OBJDUMP) -S $< > $(BINODIR)/eagle.S
else
	mkdir -p $(FIRMWAREDIR)
	@$(RM) -r $(BINODIR)/$(BIN_NAME).S $(BINODIR)/$(BIN_NAME).dump
	@$(OBJDUMP) -x -s $< > $(BINODIR)/$(BIN_NAME).dump
	@$(OBJDUMP) -S $< > $(BINODIR)/$(BIN_NAME).S
endif
	@echo ""
	@echo "VERSION: "$(GIT_VERSION)
	
ifeq ($(app), 0)
	@echo "python3 $(APPGEN_TOOL) $< 0 $(mode) $(freqdiv) $(size_map) $(app)"
	@python3 $(APPGEN_TOOL) $< 0 $(mode) $(freqdiv) $(size_map) $(app) $(BINODIR)
	@mv $(BINODIR)/eagle.app.flash.bin $(FIRMWAREDIR)/eagle.flash.bin
	@mv $(BINODIR)/eagle.app.v6.irom0text.bin $(FIRMWAREDIR)/eagle.irom0text.bin
	#@rm $(BINODIR)/eagle.app.v6.*
	@echo "No boot needed."
	@echo "Generate eagle.flash.bin and eagle.irom0text.bin successully in folder firmware."
	@echo "eagle.flash.bin-------->0x00000"
	@echo "eagle.irom0text.bin---->0x10000"
else
    ifneq ($(boot), new)
		@echo "python3 $(APPGEN_TOOL) $< 1 $(mode) $(freqdiv) $(size_map) $(app) $(BINODIR)"
		@python3 $(APPGEN_TOOL) $< 1 $(mode) $(freqdiv) $(size_map) $(app) $(BINODIR)
		@echo "Support boot_v1.1 and +"
    else
		@python3 $(APPGEN_TOOL) $< 2 $(mode) $(freqdiv) $(size_map) $(app) $(BINODIR)

    	ifeq ($(size_map), 6)
		@echo "Support boot_v1.4 and +"
        else
            ifeq ($(size_map), 5)
		@echo "Support boot_v1.4 and +"
            else
		@echo "Support boot_v1.2 and +"
            endif
        endif
    endif

	@mv $(BINODIR)/eagle.app.flash.bin $(FIRMWAREDIR)/$(BIN_NAME).bin
	@rm $(BINODIR)/eagle.app.v6.*
	@echo "Generate $(BIN_NAME).bin successully in folder bin/upgrade."
	@echo "boot.bin------------>0x00000"
	@echo "(Pick up boot.bin in folder $(SDK_DIR)/bin)"
	@echo "$(BIN_NAME).bin--->$(addr)"
endif

	@echo "!!!"
#############################################################
# Rules base
# Should be done in top-level makefile only
#

all:	.subdirs $(OBJS) $(OLIBS) $(OIMAGES) $(OBINS) $(SPECIAL_MKTARGETS)
ifndef SDK_DIR
  	$(error SDK_DIR is undefined. Generate the environment variables using 'gen_env.sh' or load them with '. env.sh')
endif
SDK_LDDIR	= $(SDK_DIR)/ld

CUSTOM_LD = $(TOP_DIR)/libs/$(notdir $(LD_FILE))
# edit the loader script to add the espfs section to the end of irom with a 4 byte alignment.
# we also adjust the sizes of the segments 'cause we need more irom0
$(CUSTOM_LD): $(LD_FILE) $(SDK_LDDIR)/eagle.rom.addr.v6.ld
	$(Q) python3 $(TOP_DIR)/tools/makeld.py $(boot) $(mode) $(freqdiv) $(size_map) $(app) $(SDK_DIR) $(TOP_DIR)/libs

WEBPAGES = $(TOP_DIR)/libs/libwebpages.a
$(WEBPAGES):
	@make -C $(TOP_DIR)/html

clean:
	$(foreach d, $(SUBDIRS), $(MAKE) -C $(d) clean;)
	$(RM) -r $(ODIR)
	$(RM) -r $(FIRMWAREDIR) $(WEBPAGES) $(CUSTOM_LD)

clobber: $(SPECIAL_CLOBBER)
	$(foreach d, $(SUBDIRS), $(MAKE) -C $(d) clobber;)
	$(RM) -r $(ODIR)

flash:
	@echo "use one of the following targets to flash the firmware"
	@echo "  make flash512k - for ESP with 512kB flash size"
	@echo "  make flash4m   - for ESP with   4MB flash size"

flash512k:
	$(MAKE) -e ESPPORT="COM3" FLASHOPTIONS="-fm qio -fs 512KB -ff 80m" flashinternal

flash4m:
	$(MAKE) -e ESPPORT="COM3" FLASHOPTIONS="-fm qio -fs 4MB -ff 80m" flashinternal

flashinternal:
ifeq ($(app), 0)
	$(ESPTOOL) --port $(ESPPORT) write_flash $(FLASHOPTIONS) 0x00000 $(FIRMWAREDIR)/eagle.flash.bin 0x10000 $(FIRMWAREDIR)/eagle.irom0text.bin
else
	$(ESPTOOL) --port $(ESPPORT) write_flash $(FLASHOPTIONS) 0x00000 $(SDK_DIR)/bin/boot_v1.7.bin $(addr) $(FIRMWAREDIR)/$(BIN_NAME).bin
endif
	
.subdirs:
	@set -e; $(foreach d, $(SUBDIRS), $(MAKE) -C $(d);)

#.subdirs:
#	$(foreach d, $(SUBDIRS), $(MAKE) -C $(d))

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),clobber)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif


$(OBJODIR)/%.o: %.c
	$(vecho) "CC $<"
	@mkdir -p $(OBJODIR);
	$(Q) $(CC) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CFLAGS)) $(COPTS_$(*F)) -o $@ -c $<

$(OBJODIR)/%.d: %.c
	$(vecho) "DEP $<"
	@mkdir -p $(OBJODIR);
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.cpp
	$(vecho) "CPP $<"
	@mkdir -p $(OBJODIR);
	$(Q) $(CXX) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CXFLAGS)) $(COPTS_$(*F)) -o $@ -c $<

$(OBJODIR)/%.d: %.cpp
	@mkdir -p $(OBJODIR);
	@echo DEPEND: $(CXX) -M $(CXFLAGS) $<
	@set -e; rm -f $@; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.s
	@mkdir -p $(OBJODIR);
	$(Q) $(CC) $(CFLAGS) -o $@ -c $<

$(OBJODIR)/%.d: %.s
	@mkdir -p $(OBJODIR); \
	set -e; rm -f $@; \
	$(Q) $(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.S
	@mkdir -p $(OBJODIR);
	$(Q) $(CC) $(CFLAGS) -D__ASSEMBLER__ -o $@ -c $<

$(OBJODIR)/%.d: %.S
	@mkdir -p $(OBJODIR); \
	set -e; rm -f $@; \
	$(Q) $(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(foreach lib,$(GEN_LIBS),$(eval $(call ShortcutRule,$(lib),$(LIBODIR))))

$(foreach image,$(GEN_IMAGES),$(eval $(call ShortcutRule,$(image),$(IMAGEODIR))))

$(foreach bin,$(GEN_BINS),$(eval $(call ShortcutRule,$(bin),$(BINODIR))))

$(foreach lib,$(GEN_LIBS),$(eval $(call MakeLibrary,$(basename $(lib)))))

$(foreach image,$(GEN_IMAGES),$(eval $(call MakeImage,$(basename $(image)))))

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

INCLUDES := $(INCLUDES) -I $(PDIR)include -I $(PDIR)include/$(TARGET)
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile
