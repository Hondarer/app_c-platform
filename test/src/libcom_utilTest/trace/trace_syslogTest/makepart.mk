# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/syslog/trace_syslog.c

# トレース共通の内部 API は stub_com_util。trace_common.c のカバレッジは traceCommonTest が担う
# stub が mock_com_util の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_com_util mock_com_util
