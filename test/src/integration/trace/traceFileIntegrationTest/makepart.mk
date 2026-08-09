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
	$(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/etw/trace_etw.c

# テスト対象が依存するソース ファイル
# stdio.c / file.c / process.c は詳細エラーの記録に error.c / result.c を使用する
# テスト固有の補助ソース (カバレッジ対象外のため TEST_SRCS には含めない)
# trace_common.c 自体の試験は traceCommonTest で行う
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/trace/trace_common.c \
	$(MYAPP_DIR)/test/libsrc/mock_com_util/trace/mock_com_util_eventlog_sink_dispose_on_unload.cc

# ライブラリの指定
LIBS += mock_libc com_util
