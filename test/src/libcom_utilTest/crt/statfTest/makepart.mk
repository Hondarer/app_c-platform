# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/path_format.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/sys_stat_format.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
