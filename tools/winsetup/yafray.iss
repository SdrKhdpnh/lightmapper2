; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=Yaf(a)Ray
AppVerName=Yaf(a)Ray 0.1.0
AppPublisherURL=http://www.yafray.org/
AppSupportURL=http://www.yafray.org/
AppUpdatesURL=http://www.yafray.org/
DefaultDirName={pf}\YafaRay
DefaultGroupName=Yaf(a)Ray
AllowNoIcons=yes
LicenseFile=E:\Archiv\coding\yafaray010\yafaray\LICENSE
InfoBeforeFile=E:\Archiv\coding\yafaray010\yafaray\README_WIN32.txt
OutputBaseFilename=yafaray
SetupIconFile=E:\Archiv\coding\yafaray010\yafaray\yafray.ico
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "win32pak\yafarayplugin.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "win32pak\yafraycore.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "win32pak\pthreadVC2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "win32pak\yafaray-xml.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "win32pak\plugins\*.dll"; DestDir: {app}\plugins; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{cm:ProgramOnTheWeb,Yaf(a)Ray}"; Filename: "http://www.yafray.org/"
Name: "{group}\{cm:UninstallProgram,Yaf(a)Ray}"; Filename: "{uninstallexe}"

[Registry]
Root: HKLM; Subkey: software\YafRay Team\YafaRay; ValueType: string; ValueName: InstallDir; Flags: uninsdeletekey; ValueData: {app}
Root: HKLM; Subkey: software\YafRay Team\; Flags: uninsdeletekeyifempty

