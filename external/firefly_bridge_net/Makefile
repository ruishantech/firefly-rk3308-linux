PROG1 := bridge_deinit.sh
PROG2 := bridge_init.sh
PROG3 := S42bridge
CONF1 := dhcpd.conf
CONF2 := bridge_config
INSTALL_DIR := $(TARGET_DIR)/usr/bin

.PHONY : install
install :
	cp -f $(PROG1) $(INSTALL_DIR)
	cp -f $(PROG2) $(INSTALL_DIR)
	cp -f $(PROG3) $(TARGET_DIR)/etc/init.d/
	mkdir -p $(TARGET_DIR)/etc/dhcp/
	cp -f $(CONF1) $(TARGET_DIR)/etc/dhcp/
	mkdir -p $(TARGET_DIR)/etc/firefly_bridge
	cp -f $(CONF2) $(TARGET_DIR)/etc/firefly_bridge/
