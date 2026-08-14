# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_descriptor.c

# ライブラリの指定
# identity 確保失敗の注入に mock_stdlib を使う
LIBS += mock_libc mock_com_util
