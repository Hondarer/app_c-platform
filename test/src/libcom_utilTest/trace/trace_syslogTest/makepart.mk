# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/syslog/trace_syslog.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace_common.c

# ライブラリの指定
LIBS += mock_libc com_util
