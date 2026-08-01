# 出力ディレクトリ
OUTPUT_DIR := $(MYAPP_DIR)/prod/cbin

# ライブラリの指定 (static library を利用)
# com_util に同梱する CLI ツール群であり、他の com_util 利用共有ライブラリを
# ロードしないため、静的リンクとしてライブラリ探索パスの設定なしに実行できるようにする。
# see: app/com_util/docs/link-policy.md
LIBS += com_util_static

ifdef PLATFORM_LINUX
    # libcom_util_static の Linux 実装は zlib / libcrypto / libdl を利用する
    LIBS += z crypto dl
endif

ifdef PLATFORM_WINDOWS
    CFLAGS   += /DCOM_UTIL_STATIC
    CXXFLAGS += /DCOM_UTIL_STATIC
    # libcom_util は both 生成で、static 側にも dllexport 付きシンボルを含む。
    # そのまま exe をリンクすると .exp と import lib も生成されるため、抑止する。
    LDFLAGS  += /NOEXP /NOIMPLIB
endif
