# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error_message.c

# 詳細エラーの記録と結果コード変換。内部 API のため実装を取り込む
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
# strerror_r は mock_libc、strcpy / 変換は mock_com_util から実ライブラリーへ委譲する
LIBS += mock_libc mock_com_util
