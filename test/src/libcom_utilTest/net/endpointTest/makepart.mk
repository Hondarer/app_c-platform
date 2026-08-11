# テスト対象のソース ファイル
# プラットフォーム別実装のため 2 本を指定する。実際にコンパイルされるのは実行環境側だけ
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/net/endpoint_linux.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/net/endpoint_windows.c

# 詳細エラーの記録先と結果コード変換。テスト側で内容を検証するため実装を取り込む
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
