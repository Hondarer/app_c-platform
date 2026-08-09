# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/runtime/sym_loader_init.c

# ライブラリの指定
# fopen / fread 等は mock_com_util 経由で実実装へ委譲する。
# cJSON は JSON 解析に直接リンクする。
LIBS += mock_libc mock_com_util cjson
