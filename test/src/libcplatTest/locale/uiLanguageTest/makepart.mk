# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/locale/ui_language.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/locale/ui_language_tag.c

# ライブラリの指定
# テスト対象が cplat_getenv、cplat_strcpy、cplat_strcasecmp を使用するため、mock_cplat が必要
# Windows の表示言語を取得する OS API の mock は mock_libc が提供する
LIBS += mock_libc mock_cplat
