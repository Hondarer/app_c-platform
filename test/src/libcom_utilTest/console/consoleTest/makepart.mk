# テスト対象のソースファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/console/console.c

# ライブラリの検索パス
LIBSDIR += \
	$(MYAPP_DIR)/prod/lib

# ライブラリの指定
LIBS += mock_libc com_util
