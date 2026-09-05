# 出力ディレクトリ
OUTPUT_DIR := $(MYAPP_DIR)/prod/lib

# ライブラリの指定
#
# cjson は JSON 設定解析、zlib は両 OS の圧縮・展開で動的に利用する。
# stdc++ は regex モジュールが C++ (std::basic_regex) で実装されているために必要。
# 共有ライブラリのリンクは $(CC) -shared (gcc) で行われ、g++ ドライバーを経由しない
# ため、libstdc++ は自動ではリンクされない。
# see: framework/makefw/makefiles/makelibsrc_c_cpp.mk
# see: app/c-platform/docs/link-policy.md
LIBS += cjson zlib
ifdef PLATFORM_LINUX
    LIBS += crypto dl stdc++
endif
