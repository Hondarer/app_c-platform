ifdef PLATFORM_WINDOWS
    # 外部関数の static 定義
    CFLAGS   += /DCOM_UTIL_STATIC
    CXXFLAGS += /DCOM_UTIL_STATIC
endif
