# テスト対象のソース ファイル
# sys_stat.c は同 TU 内で com_util_stat / com_util_mkdir を直接呼ぶため
# モック差し替えが効かない。実体ライブラリ (com_util) で検証する。
TEST_SRCS :=

# ライブラリの指定
LIBS += mock_libc com_util
