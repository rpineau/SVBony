; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "SVBony camera X2 PlugIn"
#define MyAppVersion "1.7"
#define MyAppPublisher "RTI-Zone"
#define MyAppURL "https://rti-zone.org"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{A64DB664-C06A-41F6-BA33-D5548418D15F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={code:TSXInstallDir}\Resources\Common
DefaultGroupName={#MyAppName}

; Need to customise these
; First is where you want the installer to end up
OutputDir=installer
; Next is the name of the installer
OutputBaseFilename=SVBony_X2_Installer
; Final one is the icon you would like on the installer. Comment out if not needed.
SetupIconFile=rti_zone_logo.ico
Compression=lzma
SolidCompression=yes
; We don't want users to be able to select the drectory since read by TSXInstallDir below
DisableDirPage=yes
; Uncomment this if you don't want an uninstaller.
;Uninstallable=no
CloseApplications=yes
DirExistsWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Dirs]
Name: "{app}\Plugins\CameraPlugIns";
Name: "{app}\Plugins64\CameraPlugIns";

[Files]
; WIll also need to customise these!
Source: "cameralist SVBony.txt";                        DestDir: "{app}\Miscellaneous Files"; Flags: ignoreversion
Source: "cameralist SVBony.txt";                        DestDir: "{app}\Miscellaneous Files"; DestName: "cameralist64 SVBony.txt"; Flags: ignoreversion
; 32 bit
Source: "libSVBony/Win32/Release/libSVBony.dll";        DestDir: "{app}\Plugins\CameraPlugIns"; Flags: ignoreversion
Source: "SVBonyCamSelect.ui";                           DestDir: "{app}\Plugins\CameraPlugIns"; Flags: ignoreversion
Source: "SVBonyCamera.ui";                              DestDir: "{app}\Plugins\CameraPlugIns"; Flags: ignoreversion
Source: "SVBony.png";                                   DestDir: "{app}\Plugins\CameraPlugIns"; Flags: ignoreversion
Source: "static_libs/Windows/Win32/SVBCameraSDK.dll";   DestDir: "{app}\..\..\"; Flags: ignoreversion
; 64 bit
Source: "libSVBony/x64/Release/libSVBony.dll";          DestDir: "{app}\Plugins64\CameraPlugIns"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\CameraPlugIns'))
Source: "SVBonyCamSelect.ui";                           DestDir: "{app}\Plugins64\CameraPlugIns"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\CameraPlugIns'))
Source: "SVBonyCamera.ui";                              DestDir: "{app}\Plugins64\CameraPlugIns"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\CameraPlugIns'))
Source: "SVBony.png";                                   DestDir: "{app}\Plugins64\CameraPlugIns"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\Plugins64\CameraPlugIns'))
Source: "static_libs/Windows/x64/SVBCameraSDK.dll";     DestDir: "{app}\..\..\TheSky64"; Flags: ignoreversion; Check: DirExists(ExpandConstant('{app}\..\..\TheSky64'))


; NOTE: Don't use "Flags: ignoreversion" on any shared system files
; msgBox('Do you want to install MyProg.exe to ' + ExtractFilePath(CurrentFileName) + '?', mbConfirmation, MB_YESNO)

[Code]
{* Below is a function to read TheSkyXInstallPath.txt and confirm that the directory does exist
   This is then used in the DefaultDirName above
   *}
var
  Location: String;
  LoadResult: Boolean;

function TSXInstallDir(Param: String) : String;
begin
  LoadResult := LoadStringFromFile(ExpandConstant('{userdocs}') + '\Software Bisque\TheSkyX Professional Edition\TheSkyXInstallPath.txt', Location);
  if not LoadResult then
    LoadResult := LoadStringFromFile(ExpandConstant('{userdocs}') + '\Software Bisque\TheSky Professional Edition 64\TheSkyXInstallPath.txt', Location);
    if not LoadResult then
      LoadResult := BrowseForFolder('Please locate the installation path for TheSkyX', Location, False);
      if not LoadResult then
        RaiseException('Unable to find the installation path for TheSkyX');
  if not DirExists(Location) then
    RaiseException('TheSkyX installation directory ' + Location + ' does not exist');
  Result := Location;
end;
