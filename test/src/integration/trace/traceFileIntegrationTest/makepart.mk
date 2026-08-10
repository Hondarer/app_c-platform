
# 統合テストはカバレッジの充足を目的としないため TEST_SRCS を宣言しない
# 各ソースのカバレッジは対応する単体テストが担う
# see: framework/testfw/docs/how-to-test.md の「統合テストは TEST_SRCS を宣言しない」
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/tracer.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/runtime/process.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/clock/clock.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/time.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/file.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/stdio.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/file/trace_file.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/syslog/trace_syslog.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/etw/trace_etw.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace_common.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
    $(MYAPP_DIR)/test/libsrc/mock_com_util/trace/mock_com_util_eventlog_sink_dispose_on_unload.cc

# ライブラリの指定
LIBS += mock_libc com_util
