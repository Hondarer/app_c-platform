# テスト対象のソース ファイル
# プラットフォーム別実装のため 2 本を指定する。実際にコンパイルされるのは実行環境側だけ
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/compress/compress_linux.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/compress/compress_windows.c

# 結果コード変換。テスト側で戻り値を検証するため実装を取り込む
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
ifdef PLATFORM_LINUX
    # zlib の初期化失敗を注入するため mock_zlib を使う
    LIBS += mock_zlib z
endif
