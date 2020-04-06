Set-StrictMode -Version Latest
$ErrorActionPreference = "Ignore"
$PSDefaultParameterValues['*:ErrorAction']='Ignore'

adb shell am force-stop com.beatgames.beatsaber

C:\android\sdk\ndk\21.0.6113669\ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk NDK_APPLICATION_MK=./Application.mk # V=1

adb shell am force-stop com.beatgames.beatsaber

gci -rec -file -path libs -filter *.so  |
    % { adb push $_.FullName /sdcard/Android/data/com.beatgames.beatsaber/files/mods/ }
