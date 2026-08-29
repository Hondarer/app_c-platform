# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cplat/trace/backends/etw/trace_etw.c

# ライブラリの指定
# trace_etw.c が malloc を呼ぶため、置換先の mock_libc が必要
# trace_etw.c が cplat_malloc/cplat_free を呼ぶため、mock_cplat も必要
LIBS += mock_libc mock_cplat
