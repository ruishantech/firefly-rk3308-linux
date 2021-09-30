PROG1 := wpa_ap
PROG2 := wpa_sta
PROG3 := dhcpd.conf

INSTALL_DIR := $(TARGET_DIR)/usr/bin

.PHONY : install
install :
	cp -f $(PROG1) $(INSTALL_DIR)
	cp -f $(PROG2) $(INSTALL_DIR)
	cp -f $(PROG3) $(TARGET_DIR)/etc/dhcp/
