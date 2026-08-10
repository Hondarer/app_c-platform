# 本テストは detail_out を持つ公開 API 全件が NULL を受け付け、TLS を規約どおり更新することを
# 横断的に検査する契約テストである。特定の prod ソースを対象としないため TEST_SRCS を空にしている。
# 各ソースのカバレッジは対応する単体テストが担う。
# see: framework/testfw/docs/how-to-test.md の「TEST_SRCS を空にしない」
TEST_SRCS :=

# ライブラリの指定
# 公開 API 全件を実体で呼び出すため、意図的に実ライブラリをリンクする
LIBS += com_util
