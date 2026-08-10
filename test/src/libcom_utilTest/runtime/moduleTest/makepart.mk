# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/runtime/module.c

# 詳細エラー判定とパス操作。各ソースのカバレッジは errorTest / pathNameTest が担う
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/base/result.c \
	$(MYAPP_DIR)/prod/libsrc/com_util/crt/path_name.c

# ライブラリの指定
# パス操作と詳細エラー判定は mock_com_util 経由で実実装へ委譲する
LIBS += mock_libc mock_com_util
