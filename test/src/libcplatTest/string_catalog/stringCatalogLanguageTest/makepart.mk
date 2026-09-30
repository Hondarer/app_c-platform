# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_language.c

# ライブラリの指定
LIBS += mock_cplat mock_libc
