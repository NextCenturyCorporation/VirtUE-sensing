
MessageIdTypedef=NTSTATUS

SeverityNames=(
    Success         = 0x0:STATUS_SEVERITY_SUCCESS
    Informational   = 0x1:STATUS_SEVERITY_INFORMATIONAL
    Warning         = 0x2:STATUS_SEVERITY_WARNING
    Error           = 0x3:STATUS_SEVERITY_ERROR
)

FacilityNames=(
    System      = 0x0:FACILITY_SYSTEM
    Runtime     = 0x2:FACILITY_RUNTIME
    Stubs       = 0x3:FACILITY_STUBS
    Io          = 0x4:FACILITY_IO_ERROR_CODE
)

LanguageNames=(
    English=0x409:MSG00409
    )

;//////////////////////
;// EventLog Categories
;//////////////////////

MessageId	    = 1
SymbolicName	= WVUSERVICE
Severity	    = Success
Language	    = English
Windows VirtUE Service
.

;//////////////////////
;// Events
;//////////////////////

MessageId	= +1
Severity	= Success
Facility	= Runtime
SymbolicName	= WVU_SVC_STARTED
Language	= English
%1 Service Started.
.

MessageId	= +1
Severity	= Success
Facility	= Runtime
SymbolicName	= WVU_VOL_ADDED
Language	= English
%1 Added Volume Instance %2.
.

MessageId	= +1
Severity	= Success
Facility	= Runtime
SymbolicName	= WVU_VOL_REMOVED
Language	= English
%1 Removed Volume Instance %2.
.

MessageId	= +1
Severity	= Success
Facility	= Runtime
SymbolicName	= WVU_SVC_STOPPED
Language	= English
%1 Service Stopped.
.
