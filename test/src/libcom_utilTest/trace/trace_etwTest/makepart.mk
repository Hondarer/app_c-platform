# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/trace/backends/etw/trace_etw.c

# ライブラリの指定
# trace_etw.c が malloc を呼ぶため、置換先の mock_libc が必要
# trace_etw.c が com_util_malloc/com_util_free を呼ぶため、mock_com_util も必要
LIBS += mock_libc mock_com_util
