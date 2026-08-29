# 統合テストはカバレッジの充足を目的としないため TEST_SRCS を宣言しない
# see: framework/testfw/docs/how-to-test.md の「統合テストは TEST_SRCS を宣言しない」
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cplat/sync/sync_descriptor.c \
    $(MYAPP_DIR)/prod/libsrc/cplat/sync/sync_windows.c \
    $(MYAPP_DIR)/prod/libsrc/cplat/trace/backends/etw/trace_etw.c \
    $(MYAPP_DIR)/prod/libsrc/cplat/trace/backends/etw/trace_etw_session.c \
    $(MYAPP_DIR)/prod/libsrc/cplat/crt/string.c

# ライブラリの指定
LIBS += mock_libc cplat
