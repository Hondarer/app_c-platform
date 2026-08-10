# テスト対象のソース ファイル
# Linux 専用実装のため、Windows では中身が空になる
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt_linux.c

# ライブラリの指定
# read の失敗と EINTR を注入するため mock_libc を使用する
LIBS += mock_libc mock_com_util
