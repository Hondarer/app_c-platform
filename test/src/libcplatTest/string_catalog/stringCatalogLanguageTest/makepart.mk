# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_language.c

# ライブラリの指定
# テスト対象が cplat_ui_language_get_tag と cplat_strncasecmp を使用するため、mock_cplat が必要
LIBS += mock_cplat
