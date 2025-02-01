@echo off
setlocal enabledelayedexpansion

set "global_error=0"

for %%F in (*.exe) do (
    set "filename=%%~nF"
    if /I not "!filename:~0,5!"=="speed" (
        echo Running %%F...
        "%%F"
        if errorlevel 1 (
            set "global_error=1"
        )
    )
)

endlocal
exit /b %global_error%
