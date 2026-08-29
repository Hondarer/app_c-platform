# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/runtime/sym_loader_is_default.c

# ライブラリの指定
# 解決処理は mock_cplat から実ライブラリーへ委譲する
LIBS += mock_libc mock_cplat
