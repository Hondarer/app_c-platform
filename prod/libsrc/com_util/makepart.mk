ifdef PLATFORM_WINDOWS
    # DLL エクスポート定義
    # DLL export definition
    CFLAGS   += /DCOM_UTIL_EXPORTS
    CXXFLAGS += /DCOM_UTIL_EXPORTS
endif

# 生成するライブラリは動的ライブラリのみとする。
# Linux の shared/dlopen 対応を維持するため、-ftls-model は指定しない。
LIB_TYPE = shared

ifdef PLATFORM_LINUX
    # 実行ファイルと同じディレクトリへ同梱した libcjson.so を解決する。
    LDFLAGS += -Wl,-z,origin -Wl,-rpath,'$$ORIGIN'
    # regex (C++) が実体化する libstdc++ シンボルを dyn 表へ出さない。
    # 公開 C API (com_util_* / _com_util_*) のみをエクスポートする。
    LDFLAGS += -Wl,--version-script=$(CURDIR)/exports.map
endif
