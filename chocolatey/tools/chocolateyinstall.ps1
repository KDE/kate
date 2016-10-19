$url32       = "http://download.kde.org/stable/kate/Kate-setup-16.08.2-KF5.27-32bit.exe"
$url64       = "http://download.kde.org/stable/kate/Kate-setup-16.08.2-KF5.27-64bit.exe"
$checksum32  = "1dfbc3bed44642ec22e422fcfe13201651ad11839fbd1616beba3cb5015531b8"
$checksum64  = "553c67ac8a63fd402ee63bbe54094716894232bd65af6c293dc4808b75110d97"

$installArgs  = "/S"

$params = @{
  packageName    = "kate"
  fileType       = "EXE"
  silentArgs     = $installArgs
  url            = $url32
  url64Bit       = $url64
  checksum       = $checksum32
  checksum64     = $checksum64
  checksumType   = "sha256"
  checksumType64 = "sha256"
}

$ROOT=[System.IO.Path]::GetDirectoryName($myInvocation.MyCommand.Definition)
& "$ROOT/chocolateyUninstall.ps1"

Install-ChocolateyPackage @params

[array]$key = Get-UninstallRegistryKey -SoftwareName "Kate*"
if ($key -ne $null) {
	$instDir = Split-Path -Parent $key.UninstallString
	Install-BinFile -Name "kate" -Path "$instDir/bin/kate.exe"
}
