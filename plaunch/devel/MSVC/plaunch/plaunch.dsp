# Microsoft Developer Studio Project File - Name="plaunch" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=plaunch - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "plaunch.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "plaunch.mak" CFG="plaunch - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "plaunch - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "plaunch - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""//plaunch""
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "plaunch - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "plaunch___Win32_Release"
# PROP BASE Intermediate_Dir "plaunch___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gr /Zp4 /W3 /Og /Os /I "..\.." /I "..\..\sapi" /D "NDEBUG" /D "MINIRTL" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "PLAUNCH" /FAcs /FD /Gs /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib wininet.lib libc.lib /nologo /subsystem:windows /pdb:none /map /machine:I386
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "plaunch - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "plaunch___Win32_Debug"
# PROP BASE Intermediate_Dir "plaunch___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /Zp4 /W3 /Gm /GX /ZI /Od /I "..\.." /I "..\..\sapi" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_WINDOWS" /D "PLAUNCH" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "plaunch - Win32 Release"
# Name "plaunch - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\aboutbox.c
# End Source File
# Begin Source File

SOURCE=..\..\dlgtmpl.c
# End Source File
# Begin Source File

SOURCE=..\..\entry.c
# End Source File
# Begin Source File

SOURCE=..\..\hotkey.c
# End Source File
# Begin Source File

SOURCE=..\..\launchbox.c
# End Source File
# Begin Source File

SOURCE=..\..\licensebox.c
# End Source File
# Begin Source File

SOURCE=..\..\misc.c
# End Source File
# Begin Source File

SOURCE=..\..\optionsbox.c
# End Source File
# Begin Source File

SOURCE=..\..\plaunch.c
# End Source File
# Begin Source File

SOURCE=..\..\sapi\registry.c
# End Source File
# Begin Source File

SOURCE=..\..\sapi\session.c
# End Source File
# Begin Source File

SOURCE=..\..\windowlistbox.c
# End Source File
# Begin Source File

SOURCE=..\..\winmenu.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\build.h
# End Source File
# Begin Source File

SOURCE=..\..\dlgtmpl.h
# End Source File
# Begin Source File

SOURCE=..\..\entry.h
# End Source File
# Begin Source File

SOURCE=..\..\hotkey.h
# End Source File
# Begin Source File

SOURCE=..\..\misc.h
# End Source File
# Begin Source File

SOURCE=..\..\plaunch.h
# End Source File
# Begin Source File

SOURCE=..\..\sapi\registry.h
# End Source File
# Begin Source File

SOURCE=..\..\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\sapi\session.h
# End Source File
# Begin Source File

SOURCE=..\..\winmenu.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\plaunch.rc
# End Source File
# Begin Source File

SOURCE=..\..\putty.ico
# End Source File
# End Group
# Begin Source File

SOURCE=..\..\plaunch.mft
# End Source File
# End Target
# End Project
