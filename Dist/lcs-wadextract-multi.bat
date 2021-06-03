@echo off
for %%f in (%1\*.wad.client) do (
    %~dp0\lcs-wadextract.exe %%f
)
pause
