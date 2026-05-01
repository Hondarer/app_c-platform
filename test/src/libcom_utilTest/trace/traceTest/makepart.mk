# テスト対象のソースファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
