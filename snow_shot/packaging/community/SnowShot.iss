; Community Windows x64 Inno installer. Acceptance is enforced by the release workflow.
; PREPROCVER is the official packed major/minor/revision/build value.
#if PREPROCVER != 0x07010100
  #error Inno Setup 7.1.1 required
#endif
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
#ifndef AppVersionNumeric
  #error AppVersionNumeric required
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
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
MinVersion=10.0
WizardStyle=modern dynamic windows11
DisableWelcomePage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no
DisableDirPage=auto
SetupIconFile={#AppIcon}
VersionInfoVersion={#AppVersionNumeric}.0
VersionInfoTextVersion={#AppVersionNumeric}.0
VersionInfoProductVersion={#AppVersionNumeric}.0
VersionInfoProductTextVersion={#AppVersion}
VersionInfoCompany=Snow Shot Community
VersionInfoDescription=Snow Shot installer
VersionInfoCopyright=Copyright (C) 2025-2026 mg-chao
VersionInfoProductName=Snow Shot
VersionInfoOriginalFileName=snow-shot-{#AppVersion}-windows-x64-{#Variant}.exe
UninstallDisplayIcon={app}\bin\snow_shot.exe
Uninstallable=IsFreshInstall
CreateUninstallRegKey=IsFreshInstall
; Never let a manual upgrade send shutdown requests to a live recording/editor.
CloseApplications=no
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

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "zhCN"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
Name: "zhTW"; MessagesFile: "compiler:Languages\ChineseTraditional.isl"

[CustomMessages]
en.CreateDesktop=Create a desktop shortcut
zhCN.CreateDesktop=创建桌面快捷方式
zhTW.CreateDesktop=建立桌面捷徑
en.LaunchSnow=Launch Snow Shot
zhCN.LaunchSnow=启动 Snow Shot
zhTW.LaunchSnow=啟動 Snow Shot
en.Ambiguous=Multiple Snow Shot installations were found. No files have been changed.
zhCN.Ambiguous=发现多个 Snow Shot 安装，未修改任何文件。
zhTW.Ambiguous=發現多個 Snow Shot 安裝，未修改任何檔案。
en.ScopeError=The existing installation scope or ownership record could not be verified. Choose the original installation scope. No files have been changed.
zhCN.ScopeError=无法验证现有安装范围或文件归属记录。请选择原安装范围。未修改任何文件。
zhTW.ScopeError=無法驗證現有安裝範圍或檔案歸屬記錄。請選擇原安裝範圍。未修改任何檔案。
en.UpgradeTo=Upgrade Snow Shot to
zhCN.UpgradeTo=升级 Snow Shot 至
zhTW.UpgradeTo=升級 Snow Shot 至
en.KeepOriginal=Keep the original directory, settings, and installation scope:
zhCN.KeepOriginal=保留原安装目录、设置及安装范围：
zhTW.KeepOriginal=保留原安裝目錄、設定及安裝範圍：
en.FullEdition=Full edition:
zhCN.FullEdition=完整版：
zhTW.FullEdition=完整版：
en.Upgrade=Upgrade
zhCN.Upgrade=升级
zhTW.Upgrade=升級
en.Unregistered=This directory contains an unregistered Snow Shot installation. No files have been changed.
zhCN.Unregistered=该目录存在未注册的 Snow Shot 安装，未修改任何文件。
zhTW.Unregistered=此目錄存在未註冊的 Snow Shot 安裝，未修改任何檔案。
en.OriginalDir=An upgrade must use the existing installation directory.
zhCN.OriginalDir=升级必须使用原安装目录。
zhTW.OriginalDir=升級必須使用原安裝目錄。
en.StartFailed=The upgrade could not start. The previous installation was not removed.
zhCN.StartFailed=无法启动升级，未删除原安装。
zhTW.StartFailed=無法啟動升級，未刪除原安裝。
en.AppRunning=Snow Shot is running. Finish recording or editing, exit Snow Shot and its tools, then retry.
zhCN.AppRunning=Snow Shot 正在运行。请完成录屏或编辑，退出 Snow Shot 及其工具后重试。
zhTW.AppRunning=Snow Shot 正在執行。請完成錄影或編輯，結束 Snow Shot 及其工具後重試。
en.UpgradeFailed=The upgrade failed. The transaction helper attempts to restore the previous installation.
zhCN.UpgradeFailed=升级失败，事务更新程序将尝试恢复原安装。
zhTW.UpgradeFailed=升級失敗，交易更新程式將嘗試還原原安裝。
en.CloseBeforeUninstall=Close Snow Shot and retry uninstalling.
zhCN.CloseBeforeUninstall=请退出 Snow Shot 后重试卸载。
zhTW.CloseBeforeUninstall=請結束 Snow Shot 後重試解除安裝。

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktop}"; Flags: unchecked; Check: IsFreshInstall

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
Filename: "{app}\bin\snow_shot.exe"; Description: "{cm:LaunchSnow}"; Flags: postinstall nowait skipifsilent runasoriginaluser

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
    MsgBox(CustomMessage('Ambiguous'), mbError, MB_OK)
  else if ExistingRoot <> '' then begin
    Result := FileExists(AddBackslash(ExistingRoot) + 'snow-shot-installation.json')
       and (ExistingMachine = IsAdminInstallMode);
    if not Result then
      MsgBox(CustomMessage('ScopeError'), mbError, MB_OK);
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
    WizardForm.ReadyMemo.Text := CustomMessage('UpgradeTo') + ' {#AppVersion}' + #13#10 +
      CustomMessage('KeepOriginal') + #13#10 + ExistingRoot + #13#10 +
      CustomMessage('FullEdition') + ' {#Variant}';
    WizardForm.NextButton.Caption := CustomMessage('Upgrade');
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if IsFreshInstall then begin
    if FileExists(ExpandConstant('{app}\snow-shot-installation.json')) or
       FileExists(ExpandConstant('{app}\bin\snow_shot.exe')) then
      Result := CustomMessage('Unregistered');
  end else if CompareText(ExpandConstant('{app}'), ExistingRoot) <> 0 then
    Result := CustomMessage('OriginalDir');
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
      RaiseException(CustomMessage('StartFailed'))
    else if Code = 10 then
      RaiseException(CustomMessage('AppRunning'))
    else if Code <> 0 then
      RaiseException(CustomMessage('UpgradeFailed'));
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
        RaiseException(CustomMessage('CloseBeforeUninstall'));
  end;
end;
