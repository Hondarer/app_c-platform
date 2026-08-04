# 出力ディレクトリ
OUTPUT_DIR := $(MYAPP_DIR)/prod/lib

# ライブラリの指定
#
# stdc++ は regex モジュールが C++ (std::basic_regex) で実装されているために必要。
# 共有ライブラリのリンクは $(CC) -shared (gcc) で行われ、g++ ドライバーを経由しない
# ため、libstdc++ は自動ではリンクされない。
# see: framework/makefw/makefiles/makelibsrc_c_cpp.mk
ifdef PLATFORM_LINUX
    LIBS += z crypto dl stdc++
endif
