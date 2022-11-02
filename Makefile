#########################################################
#
#	Telechips DxB Utils Library Make File 
#
#########################################################

# To build, locate this folder to ROOT/dxb/linux/dxb_library/ISDBTSWMulti2

#########################################################
#	Telechips Make File Initialize
#########################################################
# dxb_library/ISDBTSWMulti2
TOP := ../../

#########################################################
#	Read Telechips Linux Configuration File 
#########################################################
include $(TOP)config

#########################################################
#	Read Telechips Linux make rules File 
#########################################################
include $(TOP)rule.mk

#########################################################
#	Common Definitions
#########################################################
include $(TOP)common.mk

#########################################################
#	Common Path Include   
#########################################################
include $(BUILD_SYSTEM)basic_path.mk

#########################################################
#	Read  define file  
#########################################################
include	$(BUILD_SYSTEM)definition.mk

#########################################################
#	Reset Variable  
#########################################################
include $(RESET_VARS)

#########################################################
#	Version Set  
#########################################################
LOCAL_VERSION_MAJOR:=00
LOCAL_VERSION_MINOR:=01
LOCAL_VERSION := _$(LOCAL_VERSION_MAJOR).$(LOCAL_VERSION_MINOR)

#########################################################
#	Set Current Folder and Global Path 
#########################################################
LOCAL_PATH	:=./
LOCAL_SRC_PATH	:=./src/
LOCAL_MIDDLEWARE_PATH := $(TOP)../middleware/

#########################################################
#	Setting Target Folder 
#########################################################
TARGET_FOLDER := ./


#########################################################
#	Setting  Target Name 
#########################################################
STATIC	:=.a
SHARED	:=.so
LOCAL_TARGET := TCCxxxx_PNG_DEC_LIB_ANDROID_V200


#########################################################
#	Add  Src File 
#########################################################
LOCAL_SRC_FILES := 

LOCAL_SRC_FILES += $(LOCAL_SRC_PATH)TCCXXX_PNG_DEC.c


#########################################################
#	Add  Header Path 
#########################################################
LOCAL_INCLUDE_HEADERS := 

LOCAL_INCLUDE_HEADERS += $(LOCAL_PATH)include 


#########################################################
#	Add  local cflag  
#########################################################
LOCAL_CFLAGS:= $(DXB_CFLAGS)

LOCAL_CFLAGS += -fPIC
#LOCAL_CFLAGS += -O1

#########################################################
#	Add  shared lib  
#########################################################
LOCAL_SHARED_LIB := 


#########################################################
#	Add  copy lib  
#########################################################
LOCAL_COPY_LIB :=


#########################################################
#	Add  static lib  
#########################################################
LOCAL_STATIC_LIB := 


include	$(BUILD_SYSTEM)build_object.mk

#########################################################
#	Make All Function 
#########################################################
all :
	$(call build-clean-obj)
	$(call build-lib-static)

#########################################################
#	Make Install Function 
#########################################################
install : 
	$(MAKE) all 
ifdef 	LOCAL_COPY_LIB	
	$(call build-copy-src-to-dest,$(LOCAL_COPY_LIB),$(COPY_TO_LIB_PATH))
endif
	$(call build-copy-to-lib,$(LOCAL_TARGET)$(STATIC),$(LOCAL_MIDDLEWARE_PATH)common/dxb_png/lib)


#########################################################
#	Make Clean Function 
#########################################################	
clean : 
	$(call build-clean-obj)


#########################################################
#	Make Debug Function 
#########################################################	
debug : 
	$(call build-debug-print)
	@echo LOCAL_MIDDLEWARE_PATH=$(LOCAL_MIDDLEWARE_PATH)
