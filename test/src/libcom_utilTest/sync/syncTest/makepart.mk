# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_descriptor.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_linux.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_windows.c

# sync_windows.c が CreateFileU を呼ぶため、win32 ラッパーと変換ユーティリティを追加する
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/win32/file_api.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/wchar_conv.c

# ライブラリの指定
LIBS += mock_libc
