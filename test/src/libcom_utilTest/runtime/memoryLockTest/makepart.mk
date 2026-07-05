# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/runtime/memory_lock.c

# ライブラリの指定
LIBS += mock_libc com_util
