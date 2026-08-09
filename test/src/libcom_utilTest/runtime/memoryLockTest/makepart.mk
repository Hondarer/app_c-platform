# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/runtime/memory_lock.c

# テスト対象が依存するソース ファイル
# memory_lock.c が OS エラー値の共通結果コード変換関数を呼ぶため追加する
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c

# ライブラリの指定
LIBS += mock_libc com_util
