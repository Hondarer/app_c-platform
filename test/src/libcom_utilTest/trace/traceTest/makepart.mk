# テスト対象のソース ファイル
# process.c は含めない (com_util_process_get_executable_path は mock_com_util の
# モックを経由させ、デフォルト パス解決をテストで制御するため)
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/tracer.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
