# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/regex/regex_utf8.cc

# 本ソースに try/catch は無い。STL 呼び出しに付く未印の例外枝を消して gcov の C2 を充足する。
CXXFLAGS += -fno-exceptions
