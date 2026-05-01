# テスト対象のソースファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/console/console.c

# ライブラリの指定
LIBS += mock_libc com_util
