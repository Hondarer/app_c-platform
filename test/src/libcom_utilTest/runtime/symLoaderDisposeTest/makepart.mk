# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/runtime/sym_loader_dispose.c

# ハンドルを実際に取得するため解決処理を取り込む
# sym_loader_resolve.c のカバレッジは symLoaderResolveTest が担う
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/runtime/sym_loader_resolve.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
