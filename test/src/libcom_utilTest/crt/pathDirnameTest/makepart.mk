# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/path_name.c

# テスト対象が依存するソース ファイル

LIBS += mock_libc com_util
