# テスト対象のソース ファイル
# process.c は含めない (com_util_process_get_executable_path は mock_com_util の
# モックを経由させ、デフォルト パス解決をテストで制御するため)
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/tracer.c

# tracer.c が com_util_path_dirname / com_util_path_join_n を呼ぶため追加する
# (com_util_path_basename は mock_com_util 経由でモック化される)
# path_name.c は詳細エラーの記録に error.c / result.c を使用する
# trace_common.c 自体の試験は traceCommonTest で行う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace_common.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/path_name.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
