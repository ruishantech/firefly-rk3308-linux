all :
	$(CC) -o modbus_client modbus_client.c -lmodbus
	$(CC) -o modbus_server modbus_server.c -lmodbus

.PHONY : clean
clean :
	rm -f modbus_client modbus_server

.PHONY : install
install :
	cp -f modbus_client $(TARGET_DIR)/usr/bin
	cp -f modbus_server $(TARGET_DIR)/usr/bin

.PHONY: uninstall
uninstall :
	rm -f $(STAGING_DIR)/usr/bin/modbus_server
	rm -f $(STAGING_DIR)/usr/lib/modbus_client
