# テスト対象のソース ファイル
TEST_SRCS := \
    $(MYAPP_DIR)/prod/libsrc/cplat/sync/sync_linux.c \
    $(MYAPP_DIR)/prod/libsrc/cplat/sync/sync_windows.c

# ディスクリプター直列化の内部 API は stub_cplat。sync_descriptor.c のカバレッジは syncDescriptorTest が担う
# CreateFileU は mock_cplat から実ライブラリーへ委譲する
# stub が mock_cplat の公開 API を参照するため、アーカイブは stub を先に置く
LIBS += mock_libc stub_cplat mock_cplat
