# テスト対象のソース ファイル
# プラットフォーム別実装を同一テストで検証するため、Linux と Windows の両方を指定する。
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/net/endpoint_linux.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/net/endpoint_windows.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/net/socket_linux.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/net/socket_windows.c

# net 実装が利用するエラー記録と結果コードの実装をテスト実行体へ取り込む。
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# OS API と com_util の依存をモックへ置き換える。
LIBS += mock_libc mock_com_util
