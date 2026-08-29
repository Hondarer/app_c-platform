# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/prompt/pinned_prompt.c

# 結果コード変換。行編集の内部 API は stub_cplat。prompt_edit.c のカバレッジは promptEditTest が担う
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/result.c

# ライブラリの指定
# stub が mock_cplat の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_cplat mock_cplat
