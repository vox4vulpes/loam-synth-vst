; Windows installer for the whole Loam Synth Bundle.
;
; Built by installer/windows/build-installer.sh, which passes:
;   AppVersion  e.g. 1.2.0 (from the release tag)
;   StageDir    folder with VST3\<synth>.vst3 and Standalone\<synth>.exe
;   IconsFile   generated [Icons] lines, one Start-menu entry per standalone app
;
; Updating: the AppId stays fixed, so a newer installer upgrades the existing
; install in place. InitializeSetup refuses to downgrade and asks before
; reinstalling the same version. The bundle's VST3 folder is wiped first so
; synths dropped from the bundle don't linger in the DAW.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#define AppName "Loam Synth Bundle"
#define AppKey "LoamSynthBundle"
#define Vst3Dir "{commoncf64}\VST3\" + AppName

[Setup]
AppId={#AppKey}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Loam
AppPublisherURL=https://github.com/vox4vulpes/loam-synth-vst
AppSupportURL=https://github.com/vox4vulpes/loam-synth-vst/issues
AppUpdatesURL=https://github.com/vox4vulpes/loam-synth-vst/releases
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
DisableDirPage=auto
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=Loam-Synth-Bundle-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
UninstallDisplayName={#AppName}

[InstallDelete]
Type: filesandordirs; Name: "{#Vst3Dir}"
Type: files; Name: "{app}\*.exe"
Type: filesandordirs; Name: "{app}\Licenses"

[Files]
Source: "{#StageDir}\VST3\*"; DestDir: "{#Vst3Dir}"; Excludes: "*.lib,*.exp"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\Standalone\*"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#StageDir}\Licenses\*"; DestDir: "{app}\Licenses"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
#ifdef IconsFile
  #include IconsFile
#endif
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: filesandordirs; Name: "{#Vst3Dir}"

[Messages]
FinishedLabel=Setup has installed [name] on your computer.%n%nThe plugins are in Common Files\VST3\{#AppName} - rescan plugins in your DAW to pick them up.

[Code]
// Pops the next dot-separated number off S ("1.2.3" -> 1, S becomes "2.3").
// Anything non-numeric (e.g. "0-rc1") counts as 0.
function NextVersionPart(var S: String): Integer;
var
  P: Integer;
begin
  P := Pos('.', S);
  if P = 0 then
  begin
    Result := StrToIntDef(S, 0);
    S := '';
  end
  else
  begin
    Result := StrToIntDef(Copy(S, 1, P - 1), 0);
    Delete(S, 1, P);
  end;
end;

// -1 if A < B, 0 if equal, 1 if A > B (compares up to four parts).
function CompareVersions(A, B: String): Integer;
var
  I, X, Y: Integer;
begin
  Result := 0;
  for I := 1 to 4 do
  begin
    X := NextVersionPart(A);
    Y := NextVersionPart(B);
    if X <> Y then
    begin
      if X > Y then Result := 1 else Result := -1;
      Exit;
    end;
  end;
end;

function InitializeSetup(): Boolean;
var
  Installed: String;
  Cmp: Integer;
begin
  Result := True;
  if not RegQueryStringValue(HKLM, 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#AppKey}_is1',
                             'DisplayVersion', Installed) then
    Exit;

  Cmp := CompareVersions('{#AppVersion}', Installed);
  if Cmp < 0 then
  begin
    SuppressibleMsgBox(FmtMessage('A newer version of {#AppName} (%1) is already installed, so this older installer ({#AppVersion}) will not replace it.', [Installed]),
                       mbInformation, MB_OK, IDOK);
    Result := False;
  end
  else if Cmp = 0 then
    Result := SuppressibleMsgBox(FmtMessage('{#AppName} %1 is already installed. Reinstall it?', [Installed]),
                                 mbConfirmation, MB_YESNO, IDYES) = IDYES;
end;
