################################################################################
#
# gallery
#
################################################################################

GALLERY_VERSION = 1.0
GALLERY_SITE = $(TOPDIR)/../app/gallery
GALLERY_SITE_METHOD = local

GALLERY_LICENSE = Apache V2.0
GALLERY_LICENSE_FILES = NOTICE

ifeq ($(BR2_PACKAGE_DEVICE_EVB),y)
define GALLERY_CONFIGURE_CMDS
	sed -i '/^TARGET/a\DEFINES += DEVICE_EVB' $(BUILD_DIR)/gallery-$(GALLERY_VERSION)/gallery.pro
	cd $(@D); $(TARGET_MAKE_ENV) $(HOST_DIR)/bin/qmake
endef
else
define GALLERY_CONFIGURE_CMDS
	cd $(@D); $(TARGET_MAKE_ENV) $(HOST_DIR)/bin/qmake
endef
endif

define GALLERY_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) -C $(@D)
endef

define GALLERY_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/local/gallery
	cp $(BUILD_DIR)/gallery-$(GALLERY_VERSION)/conf/* $(TARGET_DIR)/usr/local/gallery/
	$(INSTALL) -D -m 0755 $(@D)/galleryView \
		$(TARGET_DIR)/usr/local/gallery/galleryView
endef

$(eval $(generic-package))
