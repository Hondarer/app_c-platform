# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt.c

# prompt.c が依存する行編集ユーティリティ
# prompt_edit.c のカバレッジは promptEditTest が担う
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt_edit.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
