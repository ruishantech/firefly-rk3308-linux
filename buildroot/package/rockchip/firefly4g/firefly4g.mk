ifeq ($(BR2_PACKAGE_EC20HOTPLUG),y)
define FIREFLY4G_INSTALL_EC20HOTPLUG
	$(INSTALL) -D -m 0755 $(EC20_SITE)/ec20.sh \
                $(TARGET_DIR)/usr/bin/ec20.sh
	$(INSTALL) -D -m 0644 $(EC20_SITE)/20-ec20.rules \
                $(TARGET_DIR)/etc/udev/rules.d/20-ec20.rules     
   
endef
endif

ifeq ($(BR2_PACKAGE_FIREFLY4G),y)
EC20_SITE=$(TOPDIR)/../external/firefly4g
define FIREFLY4G_INSTALL_TARGET_CMDS
        $(INSTALL) -D -m 0755 $(EC20_SITE)/firefly-call4g \
                $(TARGET_DIR)/usr/bin/firefly-call4g
                
	$(FIREFLY4G_INSTALL_EC20HOTPLUG)
endef
endif




$(eval $(generic-package))
