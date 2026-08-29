# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/runtime/module.c

# 詳細エラー判定。内部 API のため実装を取り込む
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/result.c

# ライブラリの指定
# パス操作は mock_cplat から実ライブラリーへ委譲する
LIBS += mock_libc mock_cplat
