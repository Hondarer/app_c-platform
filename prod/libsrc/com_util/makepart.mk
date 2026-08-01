ifdef PLATFORM_WINDOWS
    # DLL エクスポート定義
    # DLL export definition
    CFLAGS   += /DCOM_UTIL_EXPORTS
    CXXFLAGS += /DCOM_UTIL_EXPORTS
endif

# 生成されるライブラリを動的ライブラリ (shared) とする
# 未指定の場合 (デフォルト) は static
# Linux の shared/dlopen 対応を維持するため、-ftls-model は指定しない
LIB_TYPE = both
