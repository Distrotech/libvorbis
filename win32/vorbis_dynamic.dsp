# Microsoft Developer Studio Project File - Name="vorbis_dynamic" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=vorbis_dynamic - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vorbis_dynamic.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vorbis_dynamic.mak" CFG="vorbis_dynamic - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vorbis_dynamic - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vorbis_dynamic - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vorbis_dynamic - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Dynamic_Release"
# PROP Intermediate_Dir "Dynamic_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VORBIS_DYNAMIC_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VORBIS_DYNAMIC_EXPORTS" /D "_WIN32" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ogg.lib /nologo /dll /machine:I386 /out:"Dynamic_Release/vorbis.dll"

!ELSEIF  "$(CFG)" == "vorbis_dynamic - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "vorbis_dynamic___Win32_Debug"
# PROP BASE Intermediate_Dir "vorbis_dynamic___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Dynamic_Debug"
# PROP Intermediate_Dir "Dynamic_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VORBIS_DYNAMIC_EXPORTS" /YX /FD /GZ  /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VORBIS_DYNAMIC_EXPORTS" /D "_WIN32" /YX /FD /GZ  /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ogg.lib /nologo /dll /debug /machine:I386 /out:"Dynamic_Debug/vorbis.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "vorbis_dynamic - Win32 Release"
# Name "vorbis_dynamic - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\analysis.c
# End Source File
# Begin Source File

SOURCE=..\lib\block.c
# End Source File
# Begin Source File

SOURCE=..\lib\codebook.c
# End Source File
# Begin Source File

SOURCE=..\lib\envelope.c
# End Source File
# Begin Source File

SOURCE=..\lib\floor0.c
# End Source File
# Begin Source File

SOURCE=..\lib\iir.c
# End Source File
# Begin Source File

SOURCE=..\lib\info.c
# End Source File
# Begin Source File

SOURCE=..\lib\lookup.c
# End Source File
# Begin Source File

SOURCE=..\lib\lpc.c
# End Source File
# Begin Source File

SOURCE=..\lib\lsp.c
# End Source File
# Begin Source File

SOURCE=..\lib\mapping0.c
# End Source File
# Begin Source File

SOURCE=..\lib\mdct.c
# End Source File
# Begin Source File

SOURCE=..\lib\psy.c
# End Source File
# Begin Source File

SOURCE=..\lib\registry.c
# End Source File
# Begin Source File

SOURCE=..\lib\res0.c
# End Source File
# Begin Source File

SOURCE=..\lib\sharedbook.c
# End Source File
# Begin Source File

SOURCE=..\lib\smallft.c
# End Source File
# Begin Source File

SOURCE=..\lib\synthesis.c
# End Source File
# Begin Source File

SOURCE=..\lib\time0.c
# End Source File
# Begin Source File

SOURCE=..\lib\window.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\vorbis\backends.h
# End Source File
# Begin Source File

SOURCE=..\lib\bookinternal.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\codebook.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\codec.h
# End Source File
# Begin Source File

SOURCE=..\lib\envelope.h
# End Source File
# Begin Source File

SOURCE=..\lib\iir.h
# End Source File
# Begin Source File

SOURCE=..\lib\lookup.h
# End Source File
# Begin Source File

SOURCE=..\lib\lookup_data.h
# End Source File
# Begin Source File

SOURCE=..\lib\lpc.h
# End Source File
# Begin Source File

SOURCE=..\lib\lsp.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\lsp12_0.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\lsp30_0.vqh
# End Source File
# Begin Source File

SOURCE=..\lib\masking.h
# End Source File
# Begin Source File

SOURCE=..\lib\mdct.h
# End Source File
# Begin Source File

SOURCE=..\lib\misc.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\mode_A.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\mode_B.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\mode_C.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\mode_D.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\mode_E.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\modes.h
# End Source File
# Begin Source File

SOURCE=..\lib\os.h
# End Source File
# Begin Source File

SOURCE=..\lib\psy.h
# End Source File
# Begin Source File

SOURCE=..\lib\registry.h
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_128_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_128_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_128_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_128_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_128_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_128_6.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_128_7.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_128_8.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_128_9.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_160_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_160_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_160_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_160_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_160_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_160_6.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_160_7.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_160_8.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_160_9.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_192_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_192_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_192_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_192_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_192_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_256_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_256_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_256_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_256_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_256_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_350_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_350_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_350_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_350_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_1024a_350_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_128_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_128_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_128_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_128_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_128_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_160_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_160_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_160_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_160_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_160_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_192_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_192_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_192_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_192_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_192_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_256_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_256_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_256_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_256_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_256_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_350_1.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_350_2.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_350_3.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_350_4.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\res0_128a_350_5.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_1024a_128.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_1024a_160.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_1024a_192.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_1024a_256.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_1024a_350.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_128a_128.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_128a_160.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_128a_192.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_128a_256.vqh
# End Source File
# Begin Source File

SOURCE=..\include\vorbis\book\resaux0_128a_350.vqh
# End Source File
# Begin Source File

SOURCE=..\lib\scales.h
# End Source File
# Begin Source File

SOURCE=..\lib\sharedbook.h
# End Source File
# Begin Source File

SOURCE=..\lib\smallft.h
# End Source File
# Begin Source File

SOURCE=..\lib\window.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Other Files"

# PROP Default_Filter ".def"
# End Group
# End Target
# End Project
