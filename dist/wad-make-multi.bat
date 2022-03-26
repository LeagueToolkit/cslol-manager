@echo off
for /d %%f in (%1\*) do (
    "%~dp0\wad-make.exe" "%%f"
)
pause
