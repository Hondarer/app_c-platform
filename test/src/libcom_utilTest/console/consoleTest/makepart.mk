# テスト対象のソースファイル
# console.c は mock 経由でテストするため、ここには含めない
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/runtime/shutdown.c

# ライブラリの検索パス
LIBSDIR += \
	$(MYAPP_DIR)/prod/lib

# ライブラリの指定
LIBS += mock_libc mock_com_util com_util
