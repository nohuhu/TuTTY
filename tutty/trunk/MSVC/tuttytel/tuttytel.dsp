# Microsoft Developer Studio Project File - Name="tuttytel" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=tuttytel - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tuttytel.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tuttytel.mak" CFG="tuttytel - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tuttytel - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "tuttytel - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""//depot/tutty/MSVC/tuttytel""
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tuttytel - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Zp4 /W3 /GX /O1 /I "..\.." /I "..\..\windows" /I "..\..\sapi" /D RELEASE=0.60.2.0 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG" /d "SERIAL_BACKEND"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 advapi32.lib comctl32.lib comdlg32.lib gdi32.lib imm32.lib shell32.lib user32.lib winmm.lib winspool.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "tuttytel - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /Zp4 /W3 /Gm /GX /ZI /Od /I "..\.." /I "..\..\windows" /I "..\..\sapi" /D SNAPSHOT=0.60.2.0 /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "UNDERLINE_COLOUR" /D "SECONDARY_SCRIPT" /D "ATT513_TERMINAL" /D "SERIAL_BACKEND" /D "SESSION_FOLDERS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG" /d "SERIAL_BACKEND"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 advapi32.lib comctl32.lib comdlg32.lib gdi32.lib imm32.lib shell32.lib user32.lib winmm.lib winspool.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "tuttytel - Win32 Release"
# Name "tuttytel - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\.\be_nossh.c

!IF  "$(CFG)" == "tuttytel - Win32 Release"

!ELSEIF  "$(CFG)" == "tuttytel - Win32 Debug"

# ADD CPP /Zi

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\.\cmdline.c
# End Source File
# Begin Source File

SOURCE=..\..\.\config.c
# End Source File
# Begin Source File

SOURCE=..\..\.\dialog.c
# End Source File
# Begin Source File

SOURCE=..\..\.\ldisc.c
# End Source File
# Begin Source File

SOURCE=..\..\.\ldiscucs.c
# End Source File
# Begin Source File

SOURCE=..\..\.\logging.c
# End Source File
# Begin Source File

SOURCE=..\..\minibidi.c
# End Source File
# Begin Source File

SOURCE=..\..\.\misc.c
# End Source File
# Begin Source File

SOURCE=..\..\.\nocproxy.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\PickIconDialog.c
# End Source File
# Begin Source File

SOURCE=..\..\pinger.c
# End Source File
# Begin Source File

SOURCE=..\..\.\pproxy.c
# End Source File
# Begin Source File

SOURCE=..\..\.\proxy.c
# End Source File
# Begin Source File

SOURCE=..\..\.\raw.c
# End Source File
# Begin Source File

SOURCE=..\..\sapi\registry.c
# End Source File
# Begin Source File

SOURCE=..\..\.\rlogin.c
# End Source File
# Begin Source File

SOURCE=..\..\sapi\session.c
# End Source File
# Begin Source File

SOURCE=..\..\.\settings.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\sizetip.c
# End Source File
# Begin Source File

SOURCE=..\..\.\telnet.c
# End Source File
# Begin Source File

SOURCE=..\..\.\terminal.c
# End Source File
# Begin Source File

SOURCE=..\..\timing.c
# End Source File
# Begin Source File

SOURCE=..\..\.\tree234.c
# End Source File
# Begin Source File

SOURCE=..\..\.\version.c
# End Source File
# Begin Source File

SOURCE=..\..\.\wcwidth.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\wincfg.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\winctrls.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\windefs.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\windlg.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\window.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\WINHANDL.C
# End Source File
# Begin Source File

SOURCE=..\..\windows\WINHELP.C
# End Source File
# Begin Source File

SOURCE=..\..\windows\winmenu.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\winmisc.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\winnet.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\winprint.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\winprogress.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\winserial.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\winstore.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\wintime.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\winucs.c
# End Source File
# Begin Source File

SOURCE=..\..\windows\winutils.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\charset\charset.h
# End Source File
# Begin Source File

SOURCE=..\..\.\dialog.h
# End Source File
# Begin Source File

SOURCE=..\..\.\int64.h
# End Source File
# Begin Source File

SOURCE=..\..\.\ldisc.h
# End Source File
# Begin Source File

SOURCE=..\..\mac\macstuff.h
# End Source File
# Begin Source File

SOURCE=..\..\.\misc.h
# End Source File
# Begin Source File

SOURCE=..\..\.\network.h
# End Source File
# Begin Source File

SOURCE=..\..\.\proxy.h
# End Source File
# Begin Source File

SOURCE=..\..\.\putty.h
# End Source File
# Begin Source File

SOURCE=..\..\.\puttymem.h
# End Source File
# Begin Source File

SOURCE=..\..\.\puttyps.h
# End Source File
# Begin Source File

SOURCE=..\..\sapi\session.h
# End Source File
# Begin Source File

SOURCE=..\..\.\ssh.h
# End Source File
# Begin Source File

SOURCE=..\..\.\storage.h
# End Source File
# Begin Source File

SOURCE=..\..\.\terminal.h
# End Source File
# Begin Source File

SOURCE=..\..\.\tree234.h
# End Source File
# Begin Source File

SOURCE=..\..\unix\unix.h
# End Source File
# Begin Source File

SOURCE=..\..\windows\win_res.h
# End Source File
# Begin Source File

SOURCE=..\..\windows\winhelp.h
# End Source File
# Begin Source File

SOURCE=..\..\windows\winstuff.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\.\putty.ico
# End Source File
# Begin Source File

SOURCE=..\..\.\puttycfg.ico
# End Source File
# Begin Source File

SOURCE=..\..\windows\win_res.rc
# End Source File
# End Group
# End Target
# End Project
