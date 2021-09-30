PROG1 := ec200s_init.sh
PROG2 := ec200s_shutdown.sh
PROG3 := ec200s.sh
PROG4 := 21-ec200s.rules
INSTALL_DIR := $(TARGET_DIR)/usr/bin

.PHONY : install
install :
	cp -f $(PROG1) $(PROG2) $(PROG3) $(INSTALL_DIR)
	cp -f $(PROG4) $(TARGET_DIR)/etc/udev/rules.d/
