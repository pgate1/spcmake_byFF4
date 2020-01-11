@echo off

set InputMMLFile="sample.txt"
set OutputSPCFile="sample.spc"

rem ------------------------------------------------------

spcmake_byFF4.exe %InputMMLFile% %OutputSPCFile%

if %ERRORLEVEL%==0 (
   start sample.spc
   exit
) else (
   echo.
   pause
)
