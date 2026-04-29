# テスト対象のソースファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/clock/clock.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/time.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/file.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/stdio.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/file/trace_file.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/syslog/trace_syslog.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/etw/trace_etw.c

# TEST_SRCS の相対インクルード解決
INCDIR += \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/file

ifdef PLATFORM_LINUX
INCDIR += $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/syslog
endif

ifdef PLATFORM_WINDOWS
INCDIR += $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/etw
endif

# ライブラリの指定
LIBS += mock_libc com_util
