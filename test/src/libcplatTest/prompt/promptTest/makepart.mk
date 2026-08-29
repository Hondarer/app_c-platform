# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/prompt/prompt.c

# 行編集の内部 API は stub_cplat。prompt_edit.c のカバレッジは promptEditTest が担う
# stub が mock_cplat の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_cplat mock_cplat
