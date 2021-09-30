PROJECT_DIR := $(shell pwd)

BIN = minigui_desktop

include config.mk

OBJ = audioplay_dialog.o \
      browser_dialog.o \
      desktop_dialog.o \
      ffplay_ipc.o \
      hardware.o \
      main.o \
      message_dialog.o \
      parameter.o \
      pic_preview_dialog.o \
      setting_backlight_dialog.o \
      setting_dialog.o \
      setting_language_dialog.o \
      setting_screenoff_dialog.o \
      setting_systemtime_dialog.o \
      setting_themestyle_dialog.o \
      setting_volume_dialog.o \
      setting_recovery_dialog.o \
      setting_version_dialog.o \
      setting_airkiss_dialog.o \
      setting_general_dialog.o \
      input_dialog.o \
      time_input_dialog.o \
      poweroff_dialog.o \
      systime.o \
      cJSON.o \
      sysfs.o \
      system.o \

CFLAGS ?= -Wall  \
	-Wno-unused-function \
	-Wno-unused-but-set-variable \
	-Wno-unused-variable \
	-I./include \
	-lpthread -lminigui_ths -ljpeg -lpng -lm \
	-lfreetype -ldrm -lts -lDeviceIo -lasound -lrtc_demo

ifeq ($(ENABLE_VIDEO),1)
OBJ += \
      videoplay_hw_dialog.o \
      videoplay_dialog.o

CFLAGS += -lavformat -lavcodec -lswscale -lavutil -DENABLE_VIDEO
endif

ifeq ($(ENABLE_BATT),1)
CFLAGS += -DENABLE_BATT
endif

ifeq ($(ENABLE_WIFI),1)
OBJ += setting_wifi_dialog.o
CFLAGS += -DENABLE_WIFI
endif

ifeq ($(ENABLE_RK816),1)
CFLAGS += -DENABLE_RK816
endif

ifeq ($(ENABLE_4_INCH),1)
CFLAGS += -DENABLE_4_INCH
else
CFLAGS += -DENABLE_7_INCH
endif

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -rf *.o $(BIN)
