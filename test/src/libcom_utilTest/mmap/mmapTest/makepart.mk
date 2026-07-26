# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/mmap/mmap_linux.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/mmap/mmap_windows.c

# mmap_*.c が依存する crt/file.c (com_util_file)、sync/*.c (com_util_interprocess_rwlock)、
# および sync_windows.c が呼ぶ win32 ラッパーと変換ユーティリティを追加する
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/file.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/stdio.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/wchar_conv.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_descriptor.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_linux.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_windows.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/win32/file_api.c

# ライブラリの指定
LIBS += mock_libc
