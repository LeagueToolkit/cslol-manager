@echo off
for %%f in (%1\*.wad.client) do (
    "%~dp0\wad-extract.exe" "%%f"
)
pause
