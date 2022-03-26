@echo off
for %%f in (%1\*.wxy) do (
    "%~dp0\wxy-extract.exe" "%%f"
)
pause
