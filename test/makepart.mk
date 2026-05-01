# インクルードの検索パス
INCDIR += \
	$(MYAPP_DIR)/test/include \
	$(TESTFW_DIR)/gtest/include \
	$(TESTFW_DIR)/include

# ライブラリの検索パス
LIBSDIR += \
	$(MYAPP_DIR)/test/lib

ifdef PLATFORM_WINDOWS
    # 外部関数の static 定義
    CFLAGS   += /DCOM_UTIL_STATIC
    CXXFLAGS += /DCOM_UTIL_STATIC
endif
