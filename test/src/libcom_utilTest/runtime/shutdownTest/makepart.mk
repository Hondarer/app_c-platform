# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/runtime/shutdown.c

# ライブラリの指定
# shutdown.c が malloc を呼ぶため、置換先の mock_libc が必要
LIBS += mock_libc mock_com_util
