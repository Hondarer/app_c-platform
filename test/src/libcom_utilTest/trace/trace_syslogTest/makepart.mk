# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/syslog/trace_syslog.c

# テスト対象が依存するソース ファイル
# trace_common.c 自体の試験は traceCommonTest で行う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace_common.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
