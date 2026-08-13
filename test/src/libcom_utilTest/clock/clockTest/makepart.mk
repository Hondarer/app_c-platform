# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/clock/clock.c

# ライブラリの指定
# timespec 演算は mock_com_util から実ライブラリーへ委譲する
LIBS += mock_libc mock_com_util
