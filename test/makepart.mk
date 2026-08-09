ifdef PLATFORM_WINDOWS
    # 外部関数の static 定義
    CFLAGS   += /DCOM_UTIL_STATIC
    CXXFLAGS += /DCOM_UTIL_STATIC

    # mock_cjson の実シンボルを参照するため cJSON の DLL import 宣言を無効にする。
    CFLAGS   += /DCJSON_HIDE_SYMBOLS
    CXXFLAGS += /DCJSON_HIDE_SYMBOLS
endif
