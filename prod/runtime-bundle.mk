# com_util の実行時ライブラリを実行ファイルと同じディレクトリーへ配置する。
COM_UTIL_RUNTIME_OUTPUT_DIR := $(MYAPP_DIR)/prod/cbin

ifdef PLATFORM_LINUX
    COM_UTIL_RUNTIME_LIBRARY := libcom_util.so
    CJSON_RUNTIME_LIBRARY    := libcjson.so
else ifdef PLATFORM_WINDOWS
    COM_UTIL_RUNTIME_LIBRARY := libcom_util.dll
    CJSON_RUNTIME_LIBRARY    := libcjson.dll
endif

COM_UTIL_RUNTIME_SOURCE := $(APP_DIR)/com_util/prod/lib/$(COM_UTIL_RUNTIME_LIBRARY)
CJSON_RUNTIME_SOURCE    := $(APP_DIR)/cjson/prod/lib/$(CJSON_RUNTIME_LIBRARY)

.PHONY: com-util-runtime-bundle com-util-runtime-clean

com-util-runtime-bundle:
	mkdir -p "$(COM_UTIL_RUNTIME_OUTPUT_DIR)"
	cp -f "$(COM_UTIL_RUNTIME_SOURCE)" "$(COM_UTIL_RUNTIME_OUTPUT_DIR)/$(COM_UTIL_RUNTIME_LIBRARY)"
	cp -f "$(CJSON_RUNTIME_SOURCE)" "$(COM_UTIL_RUNTIME_OUTPUT_DIR)/$(CJSON_RUNTIME_LIBRARY)"

com-util-runtime-clean:
	rm -f "$(COM_UTIL_RUNTIME_OUTPUT_DIR)/$(COM_UTIL_RUNTIME_LIBRARY)"
	rm -f "$(COM_UTIL_RUNTIME_OUTPUT_DIR)/$(CJSON_RUNTIME_LIBRARY)"
