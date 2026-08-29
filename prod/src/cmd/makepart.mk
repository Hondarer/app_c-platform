# 出力ディレクトリ
OUTPUT_DIR := $(MYAPP_DIR)/prod/cbin

# ライブラリの指定
# 実行時ライブラリは prod/cbin へ同梱する。
# see: app/c-platform/docs/link-policy.md
LIBS += cplat

ifdef PLATFORM_LINUX
    LDFLAGS += -Wl,-z,origin -Wl,-rpath,'$$ORIGIN'
endif
