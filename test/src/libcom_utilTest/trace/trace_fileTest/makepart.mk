# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/file/trace_file.c

# 詳細エラーの記録。トレース共通の内部 API は stub_com_util。trace_common.c のカバレッジは traceCommonTest が担う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
# stub が mock_com_util の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_com_util mock_com_util
