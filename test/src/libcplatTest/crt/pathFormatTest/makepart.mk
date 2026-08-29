# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/crt/path_format.c

# ライブラリの指定
# path_format.c が cplat_vsnprintf を呼ぶため mock_cplat を追加する
LIBS += mock_libc mock_cplat
