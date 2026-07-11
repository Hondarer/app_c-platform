# テスト対象のソース ファイル
# process.c は含めない (com_util_process_get_executable_path は mock_com_util の
# モックを経由させ、デフォルト パス解決をテストで制御するため)
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/tracer.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/trace_common.c

# tracer.c が com_util_path_dirname / com_util_path_join_n を呼ぶため追加する
# (com_util_path_basename は mock_com_util 経由でモック化される)
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/crt/path_name.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
