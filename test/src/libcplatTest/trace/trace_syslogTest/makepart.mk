# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cplat/trace/backends/syslog/trace_syslog.c

# トレース共通の内部 API は stub_cplat。trace_common.c のカバレッジは traceCommonTest が担う
# stub が mock_cplat の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_cplat mock_cplat
