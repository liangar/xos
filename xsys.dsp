# Microsoft Developer Studio Project File - Name="xsys" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=xsys - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "xsys.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "xsys.mak" CFG="xsys - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "xsys - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "xsys - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "xsys - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "include" /I "include\xmailsys" /I "include\email" /I "include\ocidb" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "INET4" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\xsys.lib"

!ELSEIF  "$(CFG)" == "xsys - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "xsys___Win32_Debug"
# PROP BASE Intermediate_Dir "xsys___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "include" /I "include\xmailsys" /I "include\email" /I "include\ocidb" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "INET4" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"lib\xsysd.lib"

!ENDIF 

# Begin Target

# Name "xsys - Win32 Release"
# Name "xsys - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "xmailsys"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\xmailsys\Errors.cpp
# End Source File
# Begin Source File

SOURCE=.\xmailsys\MkMachDep.cpp
# End Source File
# Begin Source File

SOURCE=.\xmailsys\StrUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\xmailsys\SysDep.cpp
# End Source File
# Begin Source File

SOURCE=.\xmailsys\SysDepCommon.cpp
# End Source File
# End Group
# Begin Group "xclass"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\desenc.cpp
# End Source File
# Begin Source File

SOURCE=.\gzip.cpp
# End Source File
# Begin Source File

SOURCE=.\iniFile.cpp
# End Source File
# Begin Source File

SOURCE=.\l_str.cpp
# End Source File
# Begin Source File

SOURCE=.\lfile.cpp
# End Source File
# Begin Source File

SOURCE=.\lgenrand.cpp
# End Source File
# Begin Source File

SOURCE=.\name_value.cpp
# End Source File
# Begin Source File

SOURCE=.\nr_crypt.cpp
# End Source File
# Begin Source File

SOURCE=.\ss_service.cpp
# End Source File
# Begin Source File

SOURCE=.\xini.cpp
# End Source File
# Begin Source File

SOURCE=.\xstrcrypt.cpp
# End Source File
# Begin Source File

SOURCE=.\xsys_ftplib.cpp
# End Source File
# Begin Source File

SOURCE=.\xsys_ftplib_base.cpp
# End Source File
# Begin Source File

SOURCE=.\xtype.cpp
# End Source File
# Begin Source File

SOURCE=.\xurl.cpp
# End Source File
# End Group
# Begin Group "ocidb"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ocidb\column.cpp

!IF  "$(CFG)" == "xsys - Win32 Release"

!ELSEIF  "$(CFG)" == "xsys - Win32 Debug"

# ADD CPP /YX

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ocidb\connection.cpp
# End Source File
# Begin Source File

SOURCE=.\ocidb\error.cpp
# End Source File
# Begin Source File

SOURCE=.\ocidb\parameter.cpp
# End Source File
# Begin Source File

SOURCE=.\ocidb\resultset.cpp
# End Source File
# Begin Source File

SOURCE=.\ocidb\statement.cpp
# End Source File
# End Group
# Begin Group "email"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\email\Pop3.cpp
# End Source File
# Begin Source File

SOURCE=.\email\Smtp.cpp

!IF  "$(CFG)" == "xsys - Win32 Release"

!ELSEIF  "$(CFG)" == "xsys - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\email\uu_code.cpp
# End Source File
# Begin Source File

SOURCE=.\email\xbase64.c
# End Source File
# Begin Source File

SOURCE=.\email\xemail.cpp
# End Source File
# Begin Source File

SOURCE=.\email\xpop.cpp
# End Source File
# Begin Source File

SOURCE=.\email\xsmtp.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\con_pool.cpp
# End Source File
# Begin Source File

SOURCE=.\httpclient.cpp
# End Source File
# Begin Source File

SOURCE=.\sxml.cpp
# End Source File
# Begin Source File

SOURCE=.\xseq_buf.cpp
# End Source File
# Begin Source File

SOURCE=.\xsqlite3.cpp
# End Source File
# Begin Source File

SOURCE=.\xsqlite3_pool.cpp
# End Source File
# Begin Source File

SOURCE=.\xsys.cpp
# End Source File
# Begin Source File

SOURCE=.\xsys_tcp_server.cpp
# End Source File
# Begin Source File

SOURCE=.\xwork_dbpool_server.cpp
# End Source File
# Begin Source File

SOURCE=.\xwork_dbserver.cpp
# End Source File
# Begin Source File

SOURCE=.\xwork_server.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "email header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\email\Pop3.h
# End Source File
# Begin Source File

SOURCE=.\include\email\Smtp.h
# End Source File
# Begin Source File

SOURCE=.\include\email\uu_code.h
# End Source File
# Begin Source File

SOURCE=.\include\email\xbase64.h
# End Source File
# Begin Source File

SOURCE=.\include\email\xemail.h
# End Source File
# Begin Source File

SOURCE=.\include\email\xemail_string.h
# End Source File
# Begin Source File

SOURCE=.\include\email\xpop.h
# End Source File
# Begin Source File

SOURCE=.\include\email\xsmtp.h
# End Source File
# End Group
# Begin Group "ocidb header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\ocidb\column.h
# End Source File
# Begin Source File

SOURCE=.\include\con_pool.h
# End Source File
# Begin Source File

SOURCE=.\include\ocidb\connection.h
# End Source File
# Begin Source File

SOURCE=.\include\ocidb\datetime.inl
# End Source File
# Begin Source File

SOURCE=.\include\ocidb\error.h
# End Source File
# Begin Source File

SOURCE=.\include\ocidb\oralib.h
# End Source File
# Begin Source File

SOURCE=.\include\ocidb\parameter.h
# End Source File
# Begin Source File

SOURCE=.\include\ocidb\resultset.h
# End Source File
# Begin Source File

SOURCE=.\include\ocidb\statement.h
# End Source File
# End Group
# Begin Group "xmailsys header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\xmailsys\AppDefines.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\CommTypes.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\Errors.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\StrUtils.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\SvrConfig.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\SvrDefines.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\SysDep.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\SysInclude.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\SysLists.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\SysMachine.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\SysMacros.h
# End Source File
# Begin Source File

SOURCE=.\include\xmailsys\SysTypes.h
# End Source File
# End Group
# Begin Group "xclass header"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\des.h
# End Source File
# Begin Source File

SOURCE=.\include\gzip.h
# End Source File
# Begin Source File

SOURCE=.\include\l_str.h
# End Source File
# Begin Source File

SOURCE=.\include\lfile.h
# End Source File
# Begin Source File

SOURCE=.\include\lgenrand.h
# End Source File
# Begin Source File

SOURCE=.\include\name_value.h
# End Source File
# Begin Source File

SOURCE=.\include\nr_crypt.h
# End Source File
# Begin Source File

SOURCE=.\include\ocatb.h
# End Source File
# Begin Source File

SOURCE=.\include\ss_service.h
# End Source File
# Begin Source File

SOURCE=.\include\xdatatype.h
# End Source File
# Begin Source File

SOURCE=.\include\xlist.h
# End Source File
# Begin Source File

SOURCE=.\include\xstrcrypt.h
# End Source File
# Begin Source File

SOURCE=.\include\xurl.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\include\xsys.h
# End Source File
# Begin Source File

SOURCE=.\include\xsys_ftplib.h
# End Source File
# Begin Source File

SOURCE=.\include\xsys_oci.h
# End Source File
# Begin Source File

SOURCE=.\include\xwork_dbserver.h
# End Source File
# Begin Source File

SOURCE=.\include\xwork_server.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\import\sqlite\sqlite3\sqlite3.c
# End Source File
# End Target
# End Project
