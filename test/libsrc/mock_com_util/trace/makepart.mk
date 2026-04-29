# mock 内で参照する内部インクルード解決
INCDIR += \
	$(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/etw \
	$(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/syslog \
	$(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/file
