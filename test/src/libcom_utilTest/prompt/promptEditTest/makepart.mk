# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt_edit.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
