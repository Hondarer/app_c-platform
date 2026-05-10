# テスト対象のソースファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_linux.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_windows.c

# ライブラリの指定
LIBS += mock_libc
