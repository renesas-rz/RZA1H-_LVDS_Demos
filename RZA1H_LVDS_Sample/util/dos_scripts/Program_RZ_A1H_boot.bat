@echo off
rem * Following search code gratefully provided by TES Guiliani *
rem
rem <> Manually set path to JLink install directory here if you do not
rem    want to use the auto detect method. Make sure a backslash
rem    is at the end of the path
set BASE=C:\Program Files (x86)\SEGGER\JLink_V500k\

rem setlocal enableextensions 

    echo *****************************************************************************
    echo * WARNING !                                                                 *
    echo * This script can not be run within e2studio, please run from file explorer *    
    echo *****************************************************************************
    echo.    

if exist "%BASE%\JLink.exe" goto PATH_SET

rem <> Try to automatically detect JLink install directory
set KEYNAME=HKCU\Software\SEGGER\J-Link
set VALNAME=InstallPath
rem Check if JLink is installed first
reg query %KEYNAME% /v %VALNAME%
if not "%ERRORLEVEL%" == "0" (goto NO_PATH)
rem Query the value and then pipe it through findstr in order to find the matching line that has the value.
rem Only grab token 3 and the remainder of the line. %%b is what we are interested in here.
for /f "tokens=2,*" %%a in ('reg query %KEYNAME% /v %VALNAME% ^| findstr %VALNAME%') do (
    set BASE=%%b
)
if exist "%BASE%\JLink.exe" goto PATH_SET

:NO_PATH
chgclr 0C
echo ===================================================================
echo ERROR: Segger tools not found. Please set the path for JLink.exe 
echo If you do not have Segger tools installed please obtain them from
echo the Segger website: www.segger.com
echo ===================================================================
pause
chgclr 07
exit

:PATH_SET
rem <> extract the version number from the path
set MINJVER=V600
set JVER=%BASE:~-6%
set JVER=%JVER:~0,-1%
rem Remove '_' if present
set JVER=%JVER:_V=V%
echo Your JLINK Version is %JVER%
echo Minimum JLINK Version is %MINJVER%
echo.
if /I %JVER% GEQ %MINJVER% (goto JTAGCONF_CHECK)
chgclr 0C
echo ===================================================================
echo ERROR: You need at least JLINK verison %MINJVER%
echo ===================================================================
pause
chgclr 07
exit

:JTAGCONF_CHECK
rem * Above search code gratefully provided by TES Guiliani *

    set RawData1=TempData%random%.tmp
    set RawData2=TempData%random%.tmp
    set progdir=user_application
    set toolsdir=tools
    set programmer=%BASE%
    set programmer2=JLink.exe
    echo.    
    echo using SET_SPANSION
    echo bootloader module 
    echo using SET_SPANSION_DDR_DUAL_4B
    set SelectedBootFile=DisplayIt_QSPI_Loader_SDR_DUAL_4B.bin 
    copy /y .\%toolsdir%\program_qspi_la_head.Command .\LoadUserB.Command
    echo loadbin %toolsdir%\%SelectedBootFile%,0x18000000>> LoadUserB.Command
    copy /y .\LoadUserB.Command + %toolsdir%\program_qspi_la_tail.Command LoadUserBoot.Command    
   "%programmer%\\%programmer2%" -speed 12000 -if JTAG -JTAGConf -1,-1 -device R7S721001 -CommanderScript LoadUserBoot.Command
   
:QSPI_ProgCont    

    echo.
    echo Programmer Settings
    echo Jlink location          %programmer%%programmer2%
    echo Bootloader File         %SelectedBootFile%

:CLEAN_UP
    del /q *.Command
    pause