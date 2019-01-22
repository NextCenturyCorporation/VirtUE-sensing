cd \
sc start winvirtue
(time /T & python -m WinVirtUE debug & echo %ERRORLEVEL%) 1> winvirtue.log 2>&1
REM start /MIN cmd.exe @cmd /k "python -m WinVirtUE debug"
