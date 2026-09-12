# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_argument.c

# モジュール私有ヘッダー string_catalog.h の探索パス
# テスト ディレクトリへ引き込んだソースからは、元ディレクトリを基準に解決できないため指定する
INCDIR += \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog
