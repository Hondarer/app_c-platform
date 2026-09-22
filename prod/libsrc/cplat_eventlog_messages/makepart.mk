# Windows イベント ログのメッセージ リソース専用 DLL を生成する。
LIB_TYPE = shared
ifdef PLATFORM_WINDOWS
    # see: https://learn.microsoft.com/en-us/windows/win32/eventlog/reporting-an-event
    LDFLAGS += /NOENTRY
endif
