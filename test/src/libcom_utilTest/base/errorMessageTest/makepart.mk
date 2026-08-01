# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error_message.c

# テスト対象が依存するソース ファイル
# Windows の error_message.c は FormatMessageW の結果を
# com_util_wstr_to_utf8_alloc (wchar_conv.c) で UTF-8 へ変換する
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/wchar_conv.c
