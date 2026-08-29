# 統合テストはカバレッジの充足を目的としないため TEST_SRCS を宣言しない
# 各ソースのカバレッジは対応する単体テストが担う
# see: framework/testfw/docs/how-to-test.md の「統合テストは TEST_SRCS を宣言しない」

# ライブラリの指定
LIBS += cplat
