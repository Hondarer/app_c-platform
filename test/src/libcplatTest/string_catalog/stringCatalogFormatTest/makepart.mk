# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_format.c

# テスト対象がリンクのために必要とする依存実装
# カタログはテスト ディレクトリの偽物 (fake_catalog.c) を注入する
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_argument.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_catalog.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_language.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_render.c

# モジュール私有ヘッダー string_catalog.h の探索パス
# テスト ディレクトリへ引き込んだソースからは、元ディレクトリを基準に解決できないため指定する
INCDIR += \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog

# テスト対象の実装が cplat_snprintf を使用するため、mock_cplat が必要
LIBS += mock_cplat
