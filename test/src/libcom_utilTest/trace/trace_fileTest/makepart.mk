# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/file/trace_file.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace_common.c

# trace_file.c が com_util_path_dirname を呼ぶため追加する
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/path_name.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
