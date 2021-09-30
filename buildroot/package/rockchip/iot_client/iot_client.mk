IOT_CLIENT_VERSION = 1.0
IOT_CLIENT_VERSION = master
IOT_CLIENT_SITE_METHOD = local
IOT_CLIENT_SITE = $(TOPDIR)/../external/iot_client

define IOT_CLIENT_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/iot_client $(TARGET_DIR)/usr/bin/
	mkdir -p $(TARGET_DIR)/usr/share/iot_client
	cp -rfp $(@D)/config/* $(TARGET_DIR)/usr/share/iot_client
endef

$(eval $(generic-package))

