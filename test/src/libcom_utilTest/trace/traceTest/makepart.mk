# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/tracer.c

# 内部 API と unload 時 dispose。trace_common.c のカバレッジは traceCommonTest が担う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
    $(MYAPP_DIR)/test/libsrc/mock_com_util/trace/mock_com_util_trace_file_sink_dispose_on_unload.cc \
    $(MYAPP_DIR)/test/libsrc/mock_com_util/trace/mock_com_util_syslog_sink_dispose_on_unload.cc \
    $(MYAPP_DIR)/test/libsrc/mock_com_util/trace/mock_com_util_etw_provider_dispose_on_unload.cc

# ライブラリの指定
# stub が mock_com_util の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_com_util mock_com_util
