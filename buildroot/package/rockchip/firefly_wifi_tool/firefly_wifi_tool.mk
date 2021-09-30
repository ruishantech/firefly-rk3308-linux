FIREFLY_WIFI_TOOL_VERSION = master
FIREFLY_WIFI_TOOL_SITE_METHOD = local
FIREFLY_WIFI_TOOL_SITE = $(TOPDIR)/../external/firefly_wifi_tool

define FIREFLY_WIFI_TOOL_INSTALL_TARGET_CMDS
    $(TARGET_MAKE_ENV) $(MAKE) -C $(@D) install
endef

$(eval $(generic-package))
