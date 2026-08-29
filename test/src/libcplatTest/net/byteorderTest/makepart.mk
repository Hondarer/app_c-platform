# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/net/byteorder.c

# ライブラリの指定
LIBS += mock_libc
