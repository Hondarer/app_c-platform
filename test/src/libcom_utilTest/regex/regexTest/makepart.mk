# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/regex/regex.cc

# regex.cc が依存する UTF-8 変換はテスト ディレクトリの regexUtf8Fake.cc が差し替える
# regex_utf8.cc のホワイトボックス テストは regexUtf8Test が担う
ADD_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
LIBS += mock_libc

# 本番は例外を捕捉して結果コードへ変換する。
# Linux の gcov 計測では STL 呼び出しに付く未印の例外枝を消す。
ifdef PLATFORM_LINUX
    CXXFLAGS += -DCOM_UTIL_REGEX_NO_EXCEPTIONS
    obj/regex.o: CXXFLAGS_TEST += -fno-exceptions
endif
