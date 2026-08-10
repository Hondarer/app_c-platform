# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/unistd_format.c

# 書式展開と詳細エラーの記録。各ソースのカバレッジは
# pathFormatTest / errorTest が担う
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/path_format.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
# 書式展開は mock_com_util、実アクセス確認は mock_com_util 経由で実実装へ委譲する
LIBS += mock_libc mock_com_util
