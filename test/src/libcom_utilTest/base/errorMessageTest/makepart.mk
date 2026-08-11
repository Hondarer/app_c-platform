# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error_message.c

# テスト対象が依存するソース ファイル
# Windows の error_message.c は FormatMessageW の結果を
# com_util_wstr_to_utf8_alloc (wchar_conv.c) で UTF-8 へ変換する
# Linux の error_message.c は gai_strerror の結果を com_util_strcpy (string.c) で複写する
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/string.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/wchar_conv.c

# ライブラリの指定
# error_message.c が strerror_r を呼ぶため、置換先の mock_libc が必要
LIBS += mock_libc
