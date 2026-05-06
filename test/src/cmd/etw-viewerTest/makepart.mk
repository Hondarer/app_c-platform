# テスト対象のソースファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/src/cmd/etw-viewer/etw-viewer.c \
	$(MYAPP_DIR)/test/libsrc/mock_com_util/runtime/mock_com_util_shutdown_request_register.cc

# エントリーポイントの変更
# テスト対象のソースファイルにある main() は直接実行されず、
# テストコード内から __real_main() 経由で実行される
USE_WRAP_MAIN := 1

# テスト対象ソースのローカルヘッダーを参照する
INCDIR += \
	$(MYAPP_DIR)/prod/src/cmd/etw-viewer

# ライブラリの指定
LIBS += mock_com_util mock_libc
