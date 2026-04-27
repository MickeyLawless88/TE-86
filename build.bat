@echo off
rem ---------------------------------------------------------------
rem  build.bat  --  Build TE with Turbo C 2.0 / Turbo C++ 1.0
rem
rem  Usage:  build.bat
rem
rem  Adjust TC_DIR to match your Turbo C installation.
rem ---------------------------------------------------------------
set TC_DIR=C:\TC
set CC=%TC_DIR%\BIN\TCC.EXE
set LINK=%TC_DIR%\BIN\TLINK.EXE
set LIB=%TC_DIR%\LIB
rem  Memory model: small (fits well in DOS; use 'l' for large if needed)
set MODEL=l
echo Cleaning old object files...
del *.obj 2>nul
echo Compiling source files...
%CC% -c -m%MODEL% -O -w te_main.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_conf.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_crt.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_keys.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_ui.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_file.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_lines.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_edit.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_misc.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_error.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_macro.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_loop.c
if errorlevel 1 goto failed
%CC% -c -m%MODEL% -O -w te_mouse.c
if errorlevel 1 goto failed
echo Linking...
echo %LIB%\c0%MODEL%.obj+> te86.lnk
echo te_main.obj+>> te86.lnk
echo te_conf.obj+>> te86.lnk
echo te_crt.obj+>> te86.lnk
echo te_keys.obj+>> te86.lnk
echo te_ui.obj+>> te86.lnk
echo te_file.obj+>> te86.lnk
echo te_lines.obj+>> te86.lnk
echo te_edit.obj+>> te86.lnk
echo te_misc.obj+>> te86.lnk
echo te_error.obj+>> te86.lnk
echo te_macro.obj+>> te86.lnk
echo te_loop.obj+>> te86.lnk
echo te_mouse.obj>> te86.lnk
echo te86.exe>> te86.lnk
echo.>> te86.lnk
echo %LIB%\c%MODEL%.lib>> te86.lnk
%LINK% /x @te86.lnk
if errorlevel 1 goto failed
echo.
echo Build OK  --  te86.exe is ready.
goto end
:failed
echo.
echo Build FAILED.
:end