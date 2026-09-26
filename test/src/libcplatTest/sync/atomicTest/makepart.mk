# atomic.h は static inline 関数だけで構成され、export を持つ実体ファイル (.c) がない。
# TEST_SRCS はコンパイル単位を対象にカバレッジを計測する仕組みのため、対象ソースを指定できない。
# 本テストはヘッダーの Doxygen 契約を、ヘッダーをインクルードしたテスト翻訳単位から直接検証する。
# see: framework/testfw/docs/how-to-test.md の「TEST_SRCS を空にしない」
TEST_SRCS :=

# ライブラリの指定
# 並行性テスト (atomicConcurrencyTest.cc) が cplat_thread_create / cplat_thread_join を必要とするため、
# モックではなく実ライブラリをリンクする
LIBS += cplat
