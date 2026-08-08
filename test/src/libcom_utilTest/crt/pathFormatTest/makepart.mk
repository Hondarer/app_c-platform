# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/path_format.c

# ライブラリの指定
# path_format.c が com_util_vsnprintf を呼ぶため mock_com_util を追加する
LIBS += mock_libc mock_com_util
