# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/src/cmd/argparser-sample/argparser-sample.c

# argparser-sample が実体の argparser を利用するため追加する
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/argparser/argparser.c

# エントリ ポイントの変更
# テスト対象のソース ファイルにある main() は直接実行されず、
# テスト コード内から __real_main() 経由で実行される
USE_WRAP_MAIN := 1

# ライブラリの指定
LIBS += mock_com_util mock_libc
