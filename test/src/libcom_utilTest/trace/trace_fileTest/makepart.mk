# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/file/trace_file.c

# trace_file.c が com_util_path_dirname を呼ぶため追加する
# path_name.c は詳細エラーの記録に error.c / result.c を使用する
# trace_common.c 自体の試験は traceCommonTest で行う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace_common.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/path_name.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
