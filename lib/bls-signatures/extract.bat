@ECHO OFF
SETLOCAL EnableDelayedExpansion

:Loop
IF "%1"=="" GOTO Continue

FOR %%F in (%1) DO (

    SET LIBFILE=%%F
    SHIFT

    @ECHO !LIBFILE!

    FOR /F %%O IN ('"C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Tools\MSVC\14.29.30036\bin\Hostx64\x64\lib.exe" /LIST !LIBFILE! /NOLOGO') DO (
        @SET OBJFILE=%%O
        @ECHO !OBJFILE!
        
        SET OBJPATH=%%~dO%%~pO
        SET OBJNAME=%%~nO

        IF NOT EXIST "!OBJPATH!" md !OBJPATH!

        IF EXIST "!OBJFILE!" ECHO !OBJFILE! exists, skipping...
        IF NOT EXIST "!OBJFILE!" "C:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Tools\MSVC\14.29.30036\bin\Hostx64\x64\lib.exe" /NOLOGO !LIBFILE! "/EXTRACT:!OBJFILE!" "/OUT:!OBJFILE!"
    )
)
GOTO Loop
:Continue