# Rockchip's adbd porting for Linux
# Author : Cody Xie <cody.xie@rock-chips.com>

ifeq ($(BR2_aarch64),y)
define ADBD_INSTALL_TARGET_CMDS
$(INSTALL) -D package/rockchip/adbd/adbd \
		$(TARGET_DIR)/usr/bin/adbd
endef
endif

$(eval $(generic-package))
