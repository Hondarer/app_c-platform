# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/runtime/host.c

# テスト対象が依存するソース ファイル
# host.c が OS エラー値の共通結果コード変換関数を呼ぶため追加する
# (result.c 自体の試験は base/resultTest で行う)
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/result.c

# ライブラリの指定
LIBS += mock_libc mock_cplat
