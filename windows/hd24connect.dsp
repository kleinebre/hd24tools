# Microsoft Developer Studio Project File - Name="hd24connect" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=hd24connect - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hd24connect.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hd24connect.mak" CFG="hd24connect - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hd24connect - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "hd24connect - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hd24connect - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "hd24connect - Win32 Debug"

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
# ADD CPP /nologo /MDd /Gm /GX /ZI /Od /I "../libsndfile" /I "../fltk" /I "./setup" /I "./src/lib" /I "./src/frontend" /I "../portaudio/portaudio/pa_common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "WINDOWS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib shell32.lib gdi32.lib winmm.lib ole32.lib advapi32.lib wsock32.lib comctl32.lib comdlg32.lib fltkd.lib fltkformsd.lib fltkimagesd.lib PAStaticWMMED.lib libsndfile.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"../fltk/lib" /libpath:"../portaudio/lib" /libpath:"../libsndfile"

!ENDIF 

# Begin Target

# Name "hd24connect - Win32 Release"
# Name "hd24connect - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\src\lib\convertlib.cpp
# End Source File
# Begin Source File

SOURCE=.\src\lib\FL\Fl_Native_File_Chooser_WIN32.cxx
# End Source File
# Begin Source File

SOURCE=.\src\frontend\hd24connect.cpp
# End Source File
# Begin Source File

SOURCE=.\src\lib\hd24devicenamegenerator.cpp
# End Source File
# Begin Source File

SOURCE=.\src\lib\hd24fs.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\src\lib\FL\Fl_Native_File_Chooser_WIN32.H
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Fluid Source Files"

# PROP Default_Filter "fl"
# Begin Source File

SOURCE=.\src\frontend\dialog_choosedevice.fl

!IF  "$(CFG)" == "hd24connect - Win32 Release"

!ELSEIF  "$(CFG)" == "hd24connect - Win32 Debug"

# Begin Custom Build - Running fluid on $(InputPath)...
InputDir=.\src\frontend
WkspDir=.
InputPath=.\src\frontend\dialog_choosedevice.fl
InputName=dialog_choosedevice

BuildCmds= \
	$(WkspDir)/../fltk/fluid/fluidd -c $(InputPath) \
	mv $(InputName).cxx $(InputDir) \
	mv $(InputName).h $(InputDir) \
	

"$(InputDir)/$(InputName).cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\frontend\dialog_fromto.fl

!IF  "$(CFG)" == "hd24connect - Win32 Release"

!ELSEIF  "$(CFG)" == "hd24connect - Win32 Debug"

# Begin Custom Build - Running fluid on $(InputPath)...
InputDir=.\src\frontend
WkspDir=.
InputPath=.\src\frontend\dialog_fromto.fl
InputName=dialog_fromto

BuildCmds= \
	$(WkspDir)/../fltk/fluid/fluidd -c $(InputPath) \
	mv $(InputName).cxx $(InputDir) \
	mv $(InputName).h $(InputDir) \
	

"$(InputDir)/$(InputName).cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\frontend\dialog_rename.fl

!IF  "$(CFG)" == "hd24connect - Win32 Release"

!ELSEIF  "$(CFG)" == "hd24connect - Win32 Debug"

# Begin Custom Build - Running fluid on $(InputPath)...
InputDir=.\src\frontend
WkspDir=.
InputPath=.\src\frontend\dialog_rename.fl
InputName=dialog_rename

BuildCmds= \
	$(WkspDir)/../fltk/fluid/fluidd -c $(InputPath) \
	mv $(InputName).cxx $(InputDir) \
	mv $(InputName).h $(InputDir) \
	

"$(InputDir)/$(InputName).cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\frontend\dialog_setlocate.fl

!IF  "$(CFG)" == "hd24connect - Win32 Release"

!ELSEIF  "$(CFG)" == "hd24connect - Win32 Debug"

# Begin Custom Build - Running fluid on $(InputPath)...
InputDir=.\src\frontend
WkspDir=.
InputPath=.\src\frontend\dialog_setlocate.fl
InputName=dialog_setlocate

BuildCmds= \
	$(WkspDir)/../fltk/fluid/fluidd -c $(InputPath) \
	mv $(InputName).cxx $(InputDir) \
	mv $(InputName).h $(InputDir) \
	

"$(InputDir)/$(InputName).cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\frontend\ui_hd24connect.fl

!IF  "$(CFG)" == "hd24connect - Win32 Release"

!ELSEIF  "$(CFG)" == "hd24connect - Win32 Debug"

# Begin Custom Build - Running fluid on $(InputPath)...
InputDir=.\src\frontend
WkspDir=.
InputPath=.\src\frontend\ui_hd24connect.fl
InputName=ui_hd24connect

BuildCmds= \
	$(WkspDir)/../fltk/fluid/fluidd -c $(InputPath) \
	mv $(InputName).cxx $(InputDir) \
	mv $(InputName).h $(InputDir) \
	

"$(InputDir)/$(InputName).cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\frontend\ui_help_about.fl

!IF  "$(CFG)" == "hd24connect - Win32 Release"

!ELSEIF  "$(CFG)" == "hd24connect - Win32 Debug"

# Begin Custom Build - Running fluid on $(InputPath)...
InputDir=.\src\frontend
WkspDir=.
InputPath=.\src\frontend\ui_help_about.fl
InputName=ui_help_about

BuildCmds= \
	$(WkspDir)/../fltk/fluid/fluidd -c $(InputPath) \
	mv $(InputName).cxx $(InputDir) \
	mv $(InputName).h $(InputDir) \
	

"$(InputDir)/$(InputName).cxx" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(InputDir)/$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "Fluid Generated Source Files"

# PROP Default_Filter "cxx"
# Begin Source File

SOURCE=.\src\frontend\dialog_choosedevice.cxx
# End Source File
# Begin Source File

SOURCE=.\src\frontend\dialog_fromto.cxx
# End Source File
# Begin Source File

SOURCE=.\src\frontend\dialog_rename.cxx
# End Source File
# Begin Source File

SOURCE=.\src\frontend\dialog_setlocate.cxx
# End Source File
# Begin Source File

SOURCE=.\src\frontend\ui_hd24connect.cxx
# End Source File
# Begin Source File

SOURCE=.\src\frontend\ui_help_about.cxx
# End Source File
# End Group
# End Target
# End Project
