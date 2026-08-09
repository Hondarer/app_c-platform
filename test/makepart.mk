ifdef PLATFORM_WINDOWS
    # 製品ソースとモックをテスト実行体へ直接定義する。
    # 製品ライブラリのリンク方式は変更しない。
    CFLAGS   += /DCOM_UTIL_STATIC
    CXXFLAGS += /DCOM_UTIL_STATIC

    # mock_cjson の実シンボルを参照するため cJSON の DLL import 宣言を無効にする。
    CFLAGS   += /DCJSON_HIDE_SYMBOLS
    CXXFLAGS += /DCJSON_HIDE_SYMBOLS
endif
