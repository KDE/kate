Install-ChocolateyPackage 'kate' 'exe' '/S' 'http://download.kde.org/unstable/kate/Kate-setup-16.04.1-32bit.exe' 'http://download.kde.org/unstable/kate/Kate-setup-16.04.1-64bit.exe'
$uninstallInfo = Get-ItemProperty 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Kate\'
Generate-BinFile "kate" $uninstallInfo.DisplayIcon