@echo off
for /d %%f in (%1\*) do (
    "%~dp0\lcs-wadmake.exe" "%%f"
)
pause
