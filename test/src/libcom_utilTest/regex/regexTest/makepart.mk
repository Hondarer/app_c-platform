# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/regex/regex.cc \
    $(MYAPP_DIR)/prod/libsrc/com_util/regex/regex_utf8.cc

# regex.cc が依存するエラー詳細の記録と結果コード変換を追加する
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
LIBS += mock_libc
