# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_descriptor.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_windows.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/etw/trace_etw.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/etw/trace_etw_session.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/string.c

# ライブラリの指定
LIBS += mock_libc com_util
