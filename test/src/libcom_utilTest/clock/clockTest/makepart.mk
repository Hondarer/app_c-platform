# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/clock/clock.c

# clock.c が com_util_timespec の演算・変換関数を呼ぶため追加する
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/clock/timespec.c

# ライブラリの指定
LIBS += mock_libc mock_com_util
