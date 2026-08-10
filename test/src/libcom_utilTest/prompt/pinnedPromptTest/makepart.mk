# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/pinned_prompt.c

# pinned_prompt.c が依存する行編集ユーティリティと結果コード変換
# prompt_edit.c のカバレッジは promptEditTest が担う
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt_edit.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
