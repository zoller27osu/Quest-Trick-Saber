# Builds a .zip file for loading with BMBF
$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk
Compress-Archive -Path "./bmbfmod.json","./libs/arm64-v8a/libtricksaber.so","./libs/arm64-v8a/libbeatsaber-hook_0_5_4.so","./libs/arm64-v8a/libbs-utils.so" -DestinationPath "./TrickSaber_v0.2.1.zip" -Update
