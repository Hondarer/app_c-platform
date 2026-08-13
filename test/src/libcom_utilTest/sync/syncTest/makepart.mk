# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_linux.c \
    $(MYAPP_DIR)/prod/libsrc/com_util/sync/sync_windows.c

# ディスクリプター直列化の内部 API は stub_com_util。sync_descriptor.c のカバレッジは syncDescriptorTest が担う
# CreateFileU は mock_com_util から実ライブラリーへ委譲する
# stub が mock_com_util の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_com_util mock_com_util
