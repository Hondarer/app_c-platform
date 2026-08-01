# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/sys_stat_format.c

# テスト対象が依存するソース ファイル
# sys_stat_format.c が共有のパス書式処理を呼ぶため追加する
# (path_format.c 自体の試験は pathFormatTest で行う)
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/path_format.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
