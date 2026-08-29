# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cplat/regex/regex.cc

# regex.cc が依存する UTF-8 変換はテスト ディレクトリの regexUtf8Fake.cc が差し替える
# regex_utf8.cc のホワイトボックス テストは regexUtf8Test が担う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cplat/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/cplat/base/result.c

# ライブラリの指定
LIBS += mock_libc
