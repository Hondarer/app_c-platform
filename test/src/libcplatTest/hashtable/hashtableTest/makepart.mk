# テスト対象のソース ファイル
TEST_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/hashtable/hashtable_layout.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/hashtable/hashtable_key.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/hashtable/hashtable_arena.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/hashtable/hashtable_create.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/hashtable/hashtable_validate.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/hashtable/hashtable_modify.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/hashtable/hashtable_query.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/hashtable/hashtable_grow.c

# テスト対象ソースのモジュール私有ヘッダーを参照する
INCDIR += \
	$(MYAPP_DIR)/prod/libsrc/cplat/hashtable

# ライブラリの指定
LIBS += mock_libc mock_cplat
