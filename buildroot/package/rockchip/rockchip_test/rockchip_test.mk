# add test tool for rockchip platform
# Author : Hans Yang <yhx@rock-chips.com>

ROCKCHIP_TEST_VERSION = 20180322
ROCKCHIP_TEST_SITE_METHOD = local
ROCKCHIP_TEST_SITE = $(TOPDIR)/package/rockchip/rockchip_test/src

define ROCKCHIP_TEST_INSTALL_TARGET_CMDS
	cp -rf  $(@D)/rockchip_test  ${TARGET_DIR}/
	cp -rf $(@D)/rockchip_test_${ARCH}/* ${TARGET_DIR}/rockchip_test/ || true
	$(INSTALL) -D -m 0755 $(@D)/rockchip_test/auto_reboot/S99_auto_reboot $(TARGET_DIR)/etc/init.d/
endef

$(eval $(generic-package))
