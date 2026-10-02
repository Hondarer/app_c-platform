# テスト対象のソース ファイル
# コンパイルとフィルター オブジェクトの層だけを対象とし、カタログ定義を参照しない
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/filter/filter_compile.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/filter/filter_image.c

# モジュール私有ヘッダー filter.h と、条件式フィルターのテストが共有する filterTestSupport.h の探索パス
# テスト ディレクトリへ引き込んだソースからは、元ディレクトリを基準に解決できないため指定する
INCDIR += \
	$(MYAPP_DIR)/prod/libsrc/cplat/string_catalog/filter \
	$(MYAPP_DIR)/test/src/libcplatTest/string_catalog

# テスト対象が呼び出す標準ライブラリ関数は、include_override によって mock_libc へ差し替わる
LIBS += mock_libc mock_cplat
