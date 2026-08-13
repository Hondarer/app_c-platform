# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/fcntl_format.c

# 詳細エラーの記録。書式展開の内部 API は stub_com_util。path_format.c のカバレッジは pathFormatTest が担う
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
# stub が mock_com_util の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_com_util mock_com_util
