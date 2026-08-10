# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/regex/regex.cc

# regex.cc が依存する UTF-8 変換、エラー詳細の記録、結果コード変換を追加する
# regex_utf8.cc のホワイトボックス テストは regexUtf8Test が担う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/regex/regex_utf8.cc \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
LIBS += mock_libc
