# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_render.c

# モジュール私有ヘッダー string_catalog.h の探索パス
# テスト ディレクトリへ引き込んだソースからは、元ディレクトリを基準に解決できないため指定する
INCDIR += \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog

# テスト対象が使用する標準ライブラリ関数と cplat_snprintf のモック
# 既定では実関数へ委譲する
LIBS += mock_libc mock_cplat
