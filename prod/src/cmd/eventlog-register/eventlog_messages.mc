;// eventlog_messages.mc
;//
;// com_util EventLog backend 用のメッセージ テーブル。
;// eventlog-register.exe に埋め込み、インストール時に EventMessageFile と
;// CategoryMessageFile へ (この exe 自身を指す) 絶対パスとして登録する。
;//
;// カテゴリ メッセージ : MessageId 0x1-0x6      (レベル名 CRITICAL..DEBUG)
;// イベント メッセージ : MessageId 0x1001-0x1036
;// ReportEventW には 5 件の置換文字列を渡す:
;//   %1 メッセージ
;//   %2 実行体ファイルパス
;//   %3 ファイル識別子
;//   %4 インスタンス名
;//   %5 インスタンス識別子
;// 識別子が空のときは末尾のアンダースコアを出さない別パターンを使う。
;//
;// イベント メッセージ本文は物理 1 行で書き、改行は %n のみで表現する。
;// mc.exe は物理行末のソース改行を段落継続として半角空白に変換するため、
;// 本文を複数行に分けると 2 行目以降の行頭に空白が混入する。
;//
;// EventMessageFile と CategoryMessageFile は同一ファイルを指すため、
;// メッセージ テーブルの ID 空間は共有される。カテゴリは Windows の慣例で
;// 1..CategoryCount に固定配置する必要があるため、イベント ID は衝突を避けて
;// 0x1000 番台に置く。
;//
;// ID は trace_eventlog.c の map_level() と一致させること:
;//   識別子なし          : event_id = 0x1001 + level
;//   ファイル識別子のみ  : event_id = 0x1011 + level
;//   インスタンス識別子  : event_id = 0x1021 + level
;//   両方の識別子        : event_id = 0x1031 + level
;//   category = level + 1
;//
;// 全エントリは Severity=Success(0) / Facility=App(0) とし、合成メッセージ ID を
;// 素のコード値に一致させる。イベントのアイコン (Error/Warning/Information) は
;// ReportEventW の wType 引数で決まり、ここでの Severity とは独立する。
;//

MessageIdTypedef=DWORD

SeverityNames=(
    Success=0x0:STATUS_SEVERITY_SUCCESS
)

FacilityNames=(
    App=0x0:FACILITY_APP
)

LanguageNames=(
    Neutral=0x0:MSG00001
)

;// ===== カテゴリ メッセージ (1-6) =====

MessageId=0x1
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_CAT_CRITICAL
Language=Neutral
CRITICAL
.

MessageId=0x2
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_CAT_ERROR
Language=Neutral
ERROR
.

MessageId=0x3
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_CAT_WARNING
Language=Neutral
WARNING
.

MessageId=0x4
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_CAT_INFO
Language=Neutral
INFO
.

MessageId=0x5
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_CAT_VERBOSE
Language=Neutral
VERBOSE
.

MessageId=0x6
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_CAT_DEBUG
Language=Neutral
DEBUG
.

;// ===== イベント メッセージ: 識別子なし (0x1001-0x1006) =====

MessageId=0x1001
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_CRITICAL
Language=Neutral
%2%n%4%n%n%1%0
.

MessageId=0x1002
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_ERROR
Language=Neutral
%2%n%4%n%n%1%0
.

MessageId=0x1003
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_WARNING
Language=Neutral
%2%n%4%n%n%1%0
.

MessageId=0x1004
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_INFO
Language=Neutral
%2%n%4%n%n%1%0
.

MessageId=0x1005
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_VERBOSE
Language=Neutral
%2%n%4%n%n%1%0
.

MessageId=0x1006
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_DEBUG
Language=Neutral
%2%n%4%n%n%1%0
.

;// ===== イベント メッセージ: ファイル識別子のみ (0x1011-0x1016) =====

MessageId=0x1011
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_CRITICAL_FILE
Language=Neutral
%2_%3%n%4%n%n%1%0
.

MessageId=0x1012
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_ERROR_FILE
Language=Neutral
%2_%3%n%4%n%n%1%0
.

MessageId=0x1013
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_WARNING_FILE
Language=Neutral
%2_%3%n%4%n%n%1%0
.

MessageId=0x1014
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_INFO_FILE
Language=Neutral
%2_%3%n%4%n%n%1%0
.

MessageId=0x1015
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_VERBOSE_FILE
Language=Neutral
%2_%3%n%4%n%n%1%0
.

MessageId=0x1016
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_DEBUG_FILE
Language=Neutral
%2_%3%n%4%n%n%1%0
.

;// ===== イベント メッセージ: インスタンス識別子のみ (0x1021-0x1026) =====

MessageId=0x1021
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_CRITICAL_INSTANCE
Language=Neutral
%2%n%4_%5%n%n%1%0
.

MessageId=0x1022
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_ERROR_INSTANCE
Language=Neutral
%2%n%4_%5%n%n%1%0
.

MessageId=0x1023
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_WARNING_INSTANCE
Language=Neutral
%2%n%4_%5%n%n%1%0
.

MessageId=0x1024
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_INFO_INSTANCE
Language=Neutral
%2%n%4_%5%n%n%1%0
.

MessageId=0x1025
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_VERBOSE_INSTANCE
Language=Neutral
%2%n%4_%5%n%n%1%0
.

MessageId=0x1026
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_DEBUG_INSTANCE
Language=Neutral
%2%n%4_%5%n%n%1%0
.

;// ===== イベント メッセージ: 両方の識別子 (0x1031-0x1036) =====

MessageId=0x1031
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_CRITICAL_BOTH
Language=Neutral
%2_%3%n%4_%5%n%n%1%0
.

MessageId=0x1032
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_ERROR_BOTH
Language=Neutral
%2_%3%n%4_%5%n%n%1%0
.

MessageId=0x1033
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_WARNING_BOTH
Language=Neutral
%2_%3%n%4_%5%n%n%1%0
.

MessageId=0x1034
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_INFO_BOTH
Language=Neutral
%2_%3%n%4_%5%n%n%1%0
.

MessageId=0x1035
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_VERBOSE_BOTH
Language=Neutral
%2_%3%n%4_%5%n%n%1%0
.

MessageId=0x1036
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_DEBUG_BOTH
Language=Neutral
%2_%3%n%4_%5%n%n%1%0
.
