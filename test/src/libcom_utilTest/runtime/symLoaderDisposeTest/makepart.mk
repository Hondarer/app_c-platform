# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/runtime/sym_loader_dispose.c

# ライブラリの指定
# 解決処理は mock_com_util から実ライブラリーへ委譲する
LIBS += mock_libc mock_com_util
