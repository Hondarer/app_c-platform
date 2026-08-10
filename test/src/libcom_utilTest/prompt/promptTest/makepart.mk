# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt.c

# prompt.c が依存する行編集ユーティリティと、テスト固有のプラットフォーム層 fake
# prompt_edit.c のカバレッジは promptEditTest が担う
# 実機の端末を必要とせずキー入力を再現するため、prompt_linux.c / prompt_windows.c は
# リンクせず promptPlatformFake.cc で prompt_platform_* を差し替える
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt_edit.c \
	promptPlatformFake.cc

# ライブラリの指定
LIBS += mock_libc mock_com_util
