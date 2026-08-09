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
    # 実行ファイルと同じディレクトリーへ同梱した libcjson.so を解決する。
    LDFLAGS += -Wl,-z,origin -Wl,-rpath,'$$ORIGIN'
endif
