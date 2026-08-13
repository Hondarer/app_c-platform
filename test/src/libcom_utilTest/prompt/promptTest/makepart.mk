# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/prompt/prompt.c

# 行編集の内部 API は stub_com_util。prompt_edit.c のカバレッジは promptEditTest が担う
# stub が mock_com_util の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_com_util mock_com_util
