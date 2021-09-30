choose_rootfs :
	$(CC) -o choose_rootfs choose_rootfs.c

.PHONY : clean
clean :
	rm -f choose_rootfs

.PHONY : install
install :
	cp -f choose_rootfs $(TARGET_DIR)/usr/sbin

.PHONY: uninstall
uninstall :
	rm -f $(TARGET_DIR)/usr/sbin/choose_rootfs
