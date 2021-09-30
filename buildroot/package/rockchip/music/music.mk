################################################################################
#
# music
#
################################################################################

MUSIC_VERSION = 1.0
MUSIC_SITE = $(TOPDIR)/../app/music
MUSIC_SITE_METHOD = local

MUSIC_LICENSE = Apache V2.0
MUSIC_LICENSE_FILES = NOTICE

ifeq ($(BR2_PACKAGE_DEVICE_EVB),y)
define MUSIC_CONFIGURE_CMDS
	sed -i '/^TARGET/a\DEFINES += DEVICE_EVB' $(BUILD_DIR)/music-$(MUSIC_VERSION)/music.pro
	cd $(@D); $(TARGET_MAKE_ENV) $(HOST_DIR)/bin/qmake
endef
else
define MUSIC_CONFIGURE_CMDS
	cd $(@D); $(TARGET_MAKE_ENV) $(HOST_DIR)/bin/qmake
endef
endif

define MUSIC_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define MUSIC_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/local/music
	cp $(BUILD_DIR)/music-$(MUSIC_VERSION)/conf/* $(TARGET_DIR)/usr/local/music/
	$(INSTALL) -D -m 0755 $(@D)/musicPlayer \
		$(TARGET_DIR)/usr/local/music/musicPlayer
endef

$(eval $(generic-package))
