# 出力ディレクトリ
OUTPUT_DIR := $(MYAPP_DIR)/prod/cbin

# ライブラリの指定
# 実行時ライブラリは prod/cbin へ同梱する。
# see: app/com_util/docs/link-policy.md
LIBS += com_util

ifdef PLATFORM_LINUX
    LDFLAGS += -Wl,-z,origin -Wl,-rpath,'$$ORIGIN'
endif
