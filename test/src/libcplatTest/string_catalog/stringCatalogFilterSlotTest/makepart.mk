# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/filter/filter_slot.c

# テスト対象がリンクのために必要とする依存実装
# スロットはコンパイル、検証、説明文と、文字列カタログの書式展開を呼び出す
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/filter/filter_compile.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/filter/filter_image.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/filter/filter_describe.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_argument.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_catalog.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_format.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_language.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/string_catalog_render.c

# モジュール私有ヘッダー filter.h と string_catalog.h の探索パス
# テスト ディレクトリへ引き込んだソースからは、元ディレクトリを基準に解決できないため指定する
INCDIR += \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/filter \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog

# スロットが呼び出す同期関数の失敗を注入するため、mock_cplat が必要
# テスト対象が呼び出す標準ライブラリ関数は、include_override によって mock_libc へ差し替わる
LIBS += mock_libc mock_cplat
