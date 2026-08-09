# 出力ディレクトリ
OUTPUT_DIR := $(MYAPP_DIR)/prod/lib

# ライブラリの指定
#
# cjson は sym_loader の JSON 設定解析に利用し、動的ライブラリとしてリンクする。
# stdc++ は regex モジュールが C++ (std::basic_regex) で実装されているために必要。
# 共有ライブラリのリンクは $(CC) -shared (gcc) で行われ、g++ ドライバーを経由しない
# ため、libstdc++ は自動ではリンクされない。
# see: framework/makefw/makefiles/makelibsrc_c_cpp.mk
# see: app/com_util/docs/link-policy.md
LIBS += cjson
ifdef PLATFORM_LINUX
    LIBS += z crypto dl stdc++
endif
