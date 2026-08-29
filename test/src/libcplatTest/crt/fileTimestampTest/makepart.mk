# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/crt/file_timestamp.c

# テスト対象が依存するソース ファイル
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/result.c

# ライブラリの指定
LIBS += mock_libc mock_cplat
