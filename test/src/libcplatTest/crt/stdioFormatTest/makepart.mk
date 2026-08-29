# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/crt/stdio_format.c

# 詳細エラーの記録。書式展開の内部 API は stub_cplat。path_format.c のカバレッジは pathFormatTest が担う
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/result.c

# ライブラリの指定
# stub が mock_cplat の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_cplat mock_cplat
