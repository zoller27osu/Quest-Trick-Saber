Set-StrictMode -Version Latest
$ErrorActionPreference = "Ignore"
$PSDefaultParameterValues['*:ErrorAction']='Ignore'

adb shell am force-stop com.beatgames.beatsaber

& $PSScriptRoot/build.ps1

adb shell am force-stop com.beatgames.beatsaber

gci -rec -file -path libs -filter *bs-utils*.so  |
    % { adb push $_.FullName /sdcard/Android/data/com.beatgames.beatsaber/files/mods/ }
gci -rec -file -path libs -filter *tricksaber*.so  |
    % { adb push $_.FullName /sdcard/Android/data/com.beatgames.beatsaber/files/mods/ }
