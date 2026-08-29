# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cplat/trace/tracer.c

# 内部 API と unload 時 dispose。trace_common.c のカバレッジは traceCommonTest が担う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cplat/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/cplat/base/result.c \
    trace_file_internal_stub.c \
    $(MYAPP_DIR)/test/libsrc/mock_cplat/trace/mock_cplat_trace_file_sink_dispose_on_unload.cc \
    $(MYAPP_DIR)/test/libsrc/mock_cplat/trace/mock_cplat_syslog_sink_dispose_on_unload.cc \
    $(MYAPP_DIR)/test/libsrc/mock_cplat/trace/mock_cplat_etw_provider_dispose_on_unload.cc

# ライブラリの指定
# stub が mock_cplat の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_cplat mock_cplat
