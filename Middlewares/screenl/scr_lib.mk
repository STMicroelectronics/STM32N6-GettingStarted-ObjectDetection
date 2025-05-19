# following makefile variables must be defined by caller
# SCR_LIB_SCREEN_ITF : LTDC, UVCL or SPI
# SCR_LIB_RTOS : either NONE, THREADX or FREERTOS. Only use if SCR_LIB_SCREEN_ITF value is UVCL or SPI
# SCR_USBX_REL_DIR : location of usbx source code. Only use if SCR_LIB_SCREEN_ITF value is UVCL

SCR_LIB_REL_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

ifeq ($(SCR_LIB_SCREEN_ITF), UVCL)
C_SOURCES_SCR_LIB += $(SCR_LIB_REL_DIR)Src/scrl_usb.c
C_SOURCES_SCR_LIB += $(SCR_LIB_REL_DIR)Src/scrl_common.c
else ifeq ($(SCR_LIB_SCREEN_ITF), LTDC)
C_SOURCES_SCR_LIB += $(SCR_LIB_REL_DIR)Src/scrl_lcd.c
else ifeq ($(SCR_LIB_SCREEN_ITF), SPI)
C_SOURCES_SCR_LIB += $(SCR_LIB_REL_DIR)Src/scrl_spi.c
C_SOURCES_SCR_LIB += $(SCR_LIB_REL_DIR)Src/scrl_common.c
C_SOURCES_SCR_LIB += $(SCR_LIB_REL_DIR)ili9341/ili9341_reg.c
C_SOURCES_SCR_LIB += $(SCR_LIB_REL_DIR)ili9341/ili9341.c
else
$(error $(SCR_LIB_SCREEN_ITF) is not supported)
endif

C_INCLUDES_SCR_LIB += -I$(SCR_LIB_REL_DIR)Inc
ifeq ($(SCR_LIB_SCREEN_ITF), SPI)
C_INCLUDES_SCR_LIB += -I$(SCR_LIB_REL_DIR)ili9341
endif

C_DEFS_SCR_LIB += -DSCR_LIB_USE_$(SCR_LIB_SCREEN_ITF)
ifeq ($(SCR_LIB_SCREEN_ITF), UVCL)
C_DEFS_SCR_LIB += -DSCR_LIB_USE_$(SCR_LIB_RTOS)
endif
ifeq ($(SCR_LIB_SCREEN_ITF), SPI)
C_DEFS_SCR_LIB += -DSCR_LIB_USE_$(SCR_LIB_RTOS)
endif

C_SOURCES += $(C_SOURCES_SCR_LIB)
C_INCLUDES += $(C_INCLUDES_SCR_LIB)
C_DEFS += $(C_DEFS_SCR_LIB)

ifeq ($(SCR_LIB_SCREEN_ITF), UVCL)
# configure UVCL
UVC_LIB_REL_DIR := $(SCR_LIB_REL_DIR)uvcl
UVC_LIB_USB_DEVICE_STACK := USBX
UVC_LIB_RTOS := $(SCR_LIB_RTOS)
UVC_LIB_USE_DMA := NO
USBX_REL_DIR := $(SCR_USBX_REL_DIR)
include $(SCR_LIB_REL_DIR)uvcl/uvc_lib.mk
endif
