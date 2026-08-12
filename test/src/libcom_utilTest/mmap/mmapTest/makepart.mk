# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/mmap/mmap_linux.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/mmap/mmap_windows.c

# mmap_*.c が依存するエラー処理と Windows 向けの変換ユーティリティを追加する。
# com_util_file と sync の API は mock_com_util から実ライブラリーへ委譲する。
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/stdio.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/wchar_conv.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_descriptor.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_windows.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/win32/file_api.c

# Windows では com_util_file の実体を取り込む。
# 失敗注入テストは Linux 限定であり、Windows には Mock_com_util を参照する翻訳単位が
# 存在しない。MSVC は参照のない静的ライブラリー メンバーを取り込まないため、
# mock_com_util の /ALTERNATENAME 指令がリンカーへ渡らず、com_util_file_* が未解決になる。
# Linux では mock の weak シンボルが定義として働くため、取り込みは不要。
# see: framework/testfw/include/testfw.h の MOCK_WEAK_IMPL
ifdef PLATFORM_WINDOWS
ADD_SRCS += \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/file.c
endif

# ライブラリの指定
LIBS += mock_libc mock_com_util
