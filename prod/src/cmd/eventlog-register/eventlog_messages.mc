;// eventlog_messages.mc
;//
;// Message table for the com_util EventLog backend.
;// Embedded into eventlog-register.exe; registered as EventMessageFile and
;// CategoryMessageFile (pointing to the exe itself) at install time.
;//
;// Category messages : MessageId 0x1-0x6      (level names CRITICAL..DEBUG)
;// Event messages    : MessageId 0x1001-0x1036
;// ReportEventW passes five insertion strings:
;//   %1 message
;//   %2 executable path
;//   %3 file identifier
;//   %4 instance name
;//   %5 instance identifier
;// Message text variants omit the underscore when an identifier is empty.
;//
;// EventMessageFile and CategoryMessageFile point to the same file, so the
;// message-table ID space is shared. Categories must occupy IDs 1..CategoryCount
;// per the Windows convention, so event IDs are placed in the 0x1000 range to
;// avoid collision.
;//
;// Keep the IDs in sync with map_level() in trace_eventlog.c:
;//   no identifiers        : event_id = 0x1001 + level
;//   file identifier only  : event_id = 0x1011 + level
;//   instance identifier   : event_id = 0x1021 + level
;//   both identifiers      : event_id = 0x1031 + level
;//   category = level + 1
;//
;// Every entry uses Severity=Success(0) / Facility=App(0) so the composite
;// message ID equals the raw code value. The event icon (Error/Warning/
;// Information) is decided by the wType argument of ReportEventW, independent
;// of the Severity used here.
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

;// ===== Category messages (1-6) =====

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

;// ===== Event messages: no identifiers (0x1001-0x1006) =====

MessageId=0x1001
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_CRITICAL
Language=Neutral
%2
%4
%1%0
.

MessageId=0x1002
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_ERROR
Language=Neutral
%2
%4
%1%0
.

MessageId=0x1003
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_WARNING
Language=Neutral
%2
%4
%1%0
.

MessageId=0x1004
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_INFO
Language=Neutral
%2
%4
%1%0
.

MessageId=0x1005
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_VERBOSE
Language=Neutral
%2
%4
%1%0
.

MessageId=0x1006
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_DEBUG
Language=Neutral
%2
%4
%1%0
.

;// ===== Event messages: file identifier only (0x1011-0x1016) =====

MessageId=0x1011
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_CRITICAL_FILE
Language=Neutral
%2_%3
%4
%1%0
.

MessageId=0x1012
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_ERROR_FILE
Language=Neutral
%2_%3
%4
%1%0
.

MessageId=0x1013
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_WARNING_FILE
Language=Neutral
%2_%3
%4
%1%0
.

MessageId=0x1014
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_INFO_FILE
Language=Neutral
%2_%3
%4
%1%0
.

MessageId=0x1015
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_VERBOSE_FILE
Language=Neutral
%2_%3
%4
%1%0
.

MessageId=0x1016
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_DEBUG_FILE
Language=Neutral
%2_%3
%4
%1%0
.

;// ===== Event messages: instance identifier only (0x1021-0x1026) =====

MessageId=0x1021
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_CRITICAL_INSTANCE
Language=Neutral
%2
%4_%5
%1%0
.

MessageId=0x1022
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_ERROR_INSTANCE
Language=Neutral
%2
%4_%5
%1%0
.

MessageId=0x1023
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_WARNING_INSTANCE
Language=Neutral
%2
%4_%5
%1%0
.

MessageId=0x1024
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_INFO_INSTANCE
Language=Neutral
%2
%4_%5
%1%0
.

MessageId=0x1025
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_VERBOSE_INSTANCE
Language=Neutral
%2
%4_%5
%1%0
.

MessageId=0x1026
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_DEBUG_INSTANCE
Language=Neutral
%2
%4_%5
%1%0
.

;// ===== Event messages: both identifiers (0x1031-0x1036) =====

MessageId=0x1031
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_CRITICAL_BOTH
Language=Neutral
%2_%3
%4_%5
%1%0
.

MessageId=0x1032
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_ERROR_BOTH
Language=Neutral
%2_%3
%4_%5
%1%0
.

MessageId=0x1033
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_WARNING_BOTH
Language=Neutral
%2_%3
%4_%5
%1%0
.

MessageId=0x1034
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_INFO_BOTH
Language=Neutral
%2_%3
%4_%5
%1%0
.

MessageId=0x1035
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_VERBOSE_BOTH
Language=Neutral
%2_%3
%4_%5
%1%0
.

MessageId=0x1036
Severity=Success
Facility=App
SymbolicName=COM_UTIL_EVENTLOG_MSG_DEBUG_BOTH
Language=Neutral
%2_%3
%4_%5
%1%0
.
