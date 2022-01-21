@echo off
for %%f in (%1\*.wxy) do (
    "%~dp0\lcs-wxyextract.exe" "%%f"
)
pause
