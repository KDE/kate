[array]$key = Get-UninstallRegistryKey -SoftwareName "Kate*"
if ($key -ne $null) {
	Uninstall-ChocolateyPackage "kate" "EXE" "/S" -File $key.UninstallString
	$instDir = Split-Path -Parent $key.UninstallString
	Uninstall-BinFile -Name "kate" -Path "$instDir/bin/kate.exe"
}