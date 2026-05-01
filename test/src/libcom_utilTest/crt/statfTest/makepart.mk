# テスト対象のソースファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/path_format.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/sys_stat_format.c

LIBSDIR += \
	$(MYAPP_DIR)/test/lib

# ライブラリの指定
LIBS += mock_libc mock_com_util com_util
