# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/tracer.c

# tracer.c が依存する実装。カバレッジ対象は各ソースのテストが担う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace_common.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/path_name.c \
    $(MYAPP_DIR)/test/libsrc/mock_com_util/trace/mock_com_util_trace_file_sink_dispose_on_unload.cc \
    $(MYAPP_DIR)/test/libsrc/mock_com_util/trace/mock_com_util_syslog_sink_dispose_on_unload.cc \
    $(MYAPP_DIR)/test/libsrc/mock_com_util/trace/mock_com_util_etw_provider_dispose_on_unload.cc

# ライブラリの指定
LIBS += mock_libc mock_com_util
