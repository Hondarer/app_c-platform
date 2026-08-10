# テスト対象のソース ファイル
# Windows 専用実装のため、Linux では中身が空になる
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt_windows.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
