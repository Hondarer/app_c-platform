# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/src/cmd/etw-viewer/etw-viewer.c

# テスト固有の補助ソース (カバレッジ対象外のため TEST_SRCS には含めない)
ADD_SRCS := \
	$(MYAPP_DIR)/test/libsrc/mock_com_util/runtime/mock_com_util_shutdown_request_register.cc

# エントリ ポイントの変更
# テスト対象のソース ファイルにある main() は直接実行されず、
# テスト コード内から __real_main() 経由で実行される
USE_WRAP_MAIN := 1

# テスト対象ソースのローカル ヘッダーを参照する
INCDIR += \
	$(MYAPP_DIR)/prod/src/cmd/etw-viewer

# ライブラリの指定
LIBS += mock_com_util mock_libc
