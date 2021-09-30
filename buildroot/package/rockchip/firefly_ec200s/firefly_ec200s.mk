FIREFLY_EC200S_VERSION = master
FIREFLY_EC200S_SITE_METHOD = local
FIREFLY_EC200S_SITE = $(TOPDIR)/../external/firefly_ec200s

define FIREFLY_EC200S_INSTALL_TARGET_CMDS
    $(TARGET_MAKE_ENV) $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))

