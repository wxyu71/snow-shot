; Community candidate. Production packaging stays NSIS until Windows migration acceptance.
#ifndef StageDir
  #error StageDir required
#endif
#ifndef UpdateZip
  #error UpdateZip required
#endif
#ifndef LocalManifest
  #error LocalManifest required
#endif
#ifndef ManifestSha
  #error ManifestSha required
#endif
#ifndef AppVersion
  #error AppVersion required
#endif
#ifndef Variant
  #error Variant required
#endif
#ifndef OutputDir
  #error OutputDir required
#endif
#ifndef AppIcon
  #error AppIcon required
#endif

[Setup]
AppId=SnowShot
AppName=Snow Shot
AppVersion={#AppVersion}
AppPublisher=Snow Shot Community
AppPublisherURL=https://github.com/wxyu71/snow-shot
DefaultDirName={autopf}\SnowShot
DefaultGroupName=Snow Shot
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline
UsePreviousPrivileges=yes
UsePreviousAppDir=yes
UsePreviousTasks=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
WizardStyle=modern dynamic windows11
DisableWelcomePage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no
DisableDirPage=auto
SetupIconFile={#AppIcon}
UninstallDisplayIcon={app}\bin\snow_shot.exe
Uninstallable=IsFreshInstall
CreateUninstallRegKey=IsFreshInstall
CloseApplications=yes
RestartApplications=no
SetupMutex=SnowShotCommunitySetup
AllowCancelDuringInstall=no
AllowNetworkDrive=no
AllowUNCPath=no
Compression=lzma2/ultra64
SolidCompression=yes
OutputDir={#OutputDir}
OutputBaseFilename=snow-shot-{#AppVersion}-windows-x64-{#Variant}
SetupLogging=yes

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked; Check: IsFreshInstall

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Check: IsFreshInstall
Source: "{#StageDir}\bin\snow-shot-updater.exe"; DestDir: "{tmp}"; DestName: "installer-transaction.exe"; Flags: dontcopy
Source: "{#UpdateZip}"; DestDir: "{tmp}"; DestName: "community-update.zip"; Flags: dontcopy
Source: "{#LocalManifest}"; DestDir: "{tmp}"; DestName: "community-upgrade.json"; Flags: dontcopy

[Registry]
Root: HKA; Subkey: "Software\Snow Shot Community\SnowShot"; ValueType: string; ValueName: ""; ValueData: "{app}"; Flags: uninsdeletekey; Check: IsFreshInstall

[Icons]
Name: "{autoprograms}\Snow Shot"; Filename: "{app}\bin\snow_shot.exe"; Check: IsFreshInstall
Name: "{autodesktop}\Snow Shot"; Filename: "{app}\bin\snow_shot.exe"; Tasks: desktopicon; Check: IsFreshInstall

[Run]
Filename: "{app}\bin\snow_shot.exe"; Description: "Launch Snow Shot"; Flags: postinstall nowait skipifsilent runasoriginaluser

[Code]
var
  ExistingRoot: String;
  ExistingMachine: Boolean;
  Ambiguous: Boolean;

procedure ReadRegistration(RootKey: Integer; Key: String; Machine: Boolean);
var
  Path: String;
begin
  if RegQueryStringValue(RootKey, Key, '', Path) and (Path <> '') then begin
    if ExistingRoot = '' then begin
      ExistingRoot := Path;
      ExistingMachine := Machine;
    end else if (CompareText(ExistingRoot, Path) <> 0) or (ExistingMachine <> Machine) then
      Ambiguous := True;
  end;
end;

function InitializeSetup(): Boolean;
begin
  ExistingRoot := '';
  Ambiguous := False;
  ReadRegistration(HKLM64, 'Software\Snow Shot Community\SnowShot', True);
  ReadRegistration(HKCU64, 'Software\Snow Shot Community\SnowShot', False);
  ReadRegistration(HKLM64, 'Software\Snow Apps\SnowShot', True);
  ReadRegistration(HKCU64, 'Software\Snow Apps\SnowShot', False);
  ReadRegistration(HKLM32, 'Software\Snow Shot Community\SnowShot', True);
  ReadRegistration(HKCU32, 'Software\Snow Shot Community\SnowShot', False);
  ReadRegistration(HKLM32, 'Software\Snow Apps\SnowShot', True);
  ReadRegistration(HKCU32, 'Software\Snow Apps\SnowShot', False);
  Result := not Ambiguous;
  if Ambiguous then
    MsgBox('Multiple Snow Shot installations were found. No files have been changed.', mbError, MB_OK)
  else if ExistingRoot <> '' then begin
    Result := FileExists(AddBackslash(ExistingRoot) + 'snow-shot-installation.json')
       and (ExistingMachine = IsAdminInstallMode);
    if not Result then
      MsgBox('The existing installation scope or ownership record could not be verified. Choose the original installation scope. No files have been changed.', mbError, MB_OK);
  end;
end;

function IsFreshInstall(): Boolean;
begin
  Result := ExistingRoot = '';
end;

procedure InitializeWizard();
begin
  if not IsFreshInstall then
    WizardForm.DirEdit.Text := ExistingRoot;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := not IsFreshInstall and ((PageID = wpSelectDir) or (PageID = wpSelectTasks));
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if (CurPageID = wpReady) and not IsFreshInstall then begin
    WizardForm.ReadyMemo.Text := 'Upgrade Snow Shot to {#AppVersion}' + #13#10 +
      'Keep the original directory, settings, and installation scope:' + #13#10 + ExistingRoot +
      #13#10 + 'Full edition: {#Variant}';
    WizardForm.NextButton.Caption := 'Upgrade';
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if IsFreshInstall then begin
    if FileExists(ExpandConstant('{app}\snow-shot-installation.json')) or
       FileExists(ExpandConstant('{app}\bin\snow_shot.exe')) then
      Result := 'This directory contains an unregistered Snow Shot installation. No files have been changed.';
  end else if CompareText(ExpandConstant('{app}'), ExistingRoot) <> 0 then
    Result := 'An upgrade must use the existing installation directory.';
end;

procedure RegisterExtraCloseApplicationsResources();
begin
  if not IsFreshInstall then begin
    RegisterExtraCloseApplicationsResource(AddBackslash(ExistingRoot) + 'bin\snow_shot.exe');
    RegisterExtraCloseApplicationsResource(AddBackslash(ExistingRoot) + 'bin\snow-shot-updater.exe');
    RegisterExtraCloseApplicationsResource(AddBackslash(ExistingRoot) + 'bin\snow-shot-mcp.exe');
    RegisterExtraCloseApplicationsResource(AddBackslash(ExistingRoot) + 'bin\snow-ocr-process.exe');
    RegisterExtraCloseApplicationsResource(AddBackslash(ExistingRoot) + 'bin\crashpad_handler.exe');
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  Code: Integer;
begin
  if (CurStep = ssInstall) and not IsFreshInstall then begin
    { The existing uninstaller remains registered. The new helper replaces
      only owned files, updates DisplayVersion, probes startup, and rolls
      back on error. No previous uninstaller is executed. }
    ExtractTemporaryFile('installer-transaction.exe');
    ExtractTemporaryFile('community-update.zip');
    ExtractTemporaryFile('community-upgrade.json');
    if not Exec(ExpandConstant('{tmp}\installer-transaction.exe'),
      '--apply-local-installer --target "' + ExistingRoot + '"' +
      ' --archive "' + ExpandConstant('{tmp}\community-update.zip') + '"' +
      ' --manifest "' + ExpandConstant('{tmp}\community-upgrade.json') + '"' +
      ' --manifest-sha256 {#ManifestSha}', '', SW_HIDE, ewWaitUntilTerminated, Code) then
      RaiseException('The upgrade could not start. The previous installation was not removed.')
    else if Code <> 0 then
      RaiseException('The upgrade failed. The transaction helper attempts to restore the previous installation.');
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Code: Integer;
begin
  if CurUninstallStep = usUninstall then begin
    if FileExists(ExpandConstant('{app}\bin\snow-shot-updater.exe')) then
      if not Exec(ExpandConstant('{app}\bin\snow-shot-updater.exe'),
        '--uninstall --target "' + ExpandConstant('{app}') + '"',
        '', SW_HIDE, ewWaitUntilTerminated, Code) or (Code <> 0) then
        RaiseException('Close Snow Shot and retry uninstalling.');
  end;
end;
