FIREFLY_BRIDGE_NET_VERSION = master
FIREFLY_BRIDGE_NET_SITE_METHOD = local
FIREFLY_BRIDGE_NET_SITE = $(TOPDIR)/../external/firefly_bridge_net

define FIREFLY_BRIDGE_NET_INSTALL_TARGET_CMDS
    $(TARGET_MAKE_ENV) $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
