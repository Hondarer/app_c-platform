# テスト対象のソースファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/src/cmd/trace-cli/trace-cli.c

# エントリーポイントの変更
# テスト対象のソースファイルにある main() は直接実行されず、
# テストコード内から __real_main() 経由で実行される
USE_WRAP_MAIN := 1

# テスト対象ソースのローカルヘッダーを参照する
INCDIR += \
	$(MYAPP_DIR)/prod/src/cmd/trace-cli

# ライブラリの指定
LIBS += mock_com_util mock_libc
