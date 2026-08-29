# c-platform の実行時ライブラリを実行ファイルと同じディレクトリへ配置する。
CPLAT_RUNTIME_OUTPUT_DIR := $(MYAPP_DIR)/prod/cbin

ifdef PLATFORM_LINUX
    CPLAT_RUNTIME_LIBRARY := libcplat.so
    CJSON_RUNTIME_LIBRARY    := libcjson.so
else ifdef PLATFORM_WINDOWS
    CPLAT_RUNTIME_LIBRARY := libcplat.dll
    CJSON_RUNTIME_LIBRARY    := libcjson.dll
endif

CPLAT_RUNTIME_SOURCE := $(APP_DIR)/c-platform/prod/lib/$(CPLAT_RUNTIME_LIBRARY)
CJSON_RUNTIME_SOURCE    := $(APP_DIR)/cjson/prod/lib/$(CJSON_RUNTIME_LIBRARY)

.PHONY: c-platform-runtime-bundle c-platform-runtime-clean

c-platform-runtime-bundle:
	mkdir -p "$(CPLAT_RUNTIME_OUTPUT_DIR)"
	cp -f "$(CPLAT_RUNTIME_SOURCE)" "$(CPLAT_RUNTIME_OUTPUT_DIR)/$(CPLAT_RUNTIME_LIBRARY)"
	cp -f "$(CJSON_RUNTIME_SOURCE)" "$(CPLAT_RUNTIME_OUTPUT_DIR)/$(CJSON_RUNTIME_LIBRARY)"

c-platform-runtime-clean:
	rm -f "$(CPLAT_RUNTIME_OUTPUT_DIR)/$(CPLAT_RUNTIME_LIBRARY)"
	rm -f "$(CPLAT_RUNTIME_OUTPUT_DIR)/$(CJSON_RUNTIME_LIBRARY)"
