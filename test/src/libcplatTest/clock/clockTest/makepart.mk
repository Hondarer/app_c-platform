# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cplat/clock/clock.c

# ライブラリの指定
# timespec 演算は mock_cplat から実ライブラリーへ委譲する
LIBS += mock_libc mock_cplat
