# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/stdio.c

# テスト対象が依存するソース ファイル

# ライブラリの指定
LIBS += mock_libc com_util
