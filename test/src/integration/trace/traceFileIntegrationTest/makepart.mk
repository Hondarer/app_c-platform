# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/tracer.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/runtime/process.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/clock/clock.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/time.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/file.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/stdio.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/file/trace_file.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/syslog/trace_syslog.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/etw/trace_etw.c \
    $(MYAPP_DIR)/test/libsrc/mock_com_util/trace/mock_com_util_eventlog_sink_dispose_on_unload.cc

# ライブラリの指定
LIBS += mock_libc com_util
