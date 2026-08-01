# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/src/cmd/compress-cli/compress-cli.c

# テスト対象が依存するソース ファイル
# com_util_error_message() は mock_com_util の対象外のため、実体を取り込む
# (Windows の error_message.c は wchar_conv.c の UTF-8 変換を使用する)
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error_message.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/wchar_conv.c

# エントリ ポイントの変更
# テスト対象のソース ファイルにある main() は直接実行されず、
# テスト コード内から __real_main() 経由で実行される
USE_WRAP_MAIN := 1

# テスト対象ソースのローカル ヘッダーを参照する
INCDIR += \
	$(MYAPP_DIR)/prod/src/cmd/compress-cli

# ライブラリの指定
LIBS += mock_com_util mock_libc
