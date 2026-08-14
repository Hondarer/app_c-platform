# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/net/byteorder.c

# ライブラリの指定
LIBS += mock_libc
