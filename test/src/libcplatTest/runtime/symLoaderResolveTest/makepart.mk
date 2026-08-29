# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/runtime/sym_loader_resolve.c

# ライブラリの指定
# ロック生成と文字列連結は mock_cplat 経由で実実装へ委譲する
LIBS += mock_libc mock_cplat
