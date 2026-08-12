# テスト対象のソース ファイル
# プラットフォーム別実装のため 2 本を指定する。実際にコンパイルされるのは実行環境側だけ
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/mmap/mmap_linux.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/mmap/mmap_windows.c

# 詳細エラーの記録と結果コード変換。内部 API のため実装を取り込む
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
