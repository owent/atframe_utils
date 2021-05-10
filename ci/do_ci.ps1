$PSDefaultParameterValues['*:Encoding'] = 'UTF-8'

$OutputEncoding = [System.Text.UTF8Encoding]::new()

$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Definition
$WORK_DIR = Get-Location

Set-Location "$SCRIPT_DIR/.."
$RUN_MODE = $args[0]

if ( $RUN_MODE -eq "msvc.2019.test" ) {
  New-Item -Path "build_jobs_ci" -ItemType "directory" -Force 
  Set-Location "build_jobs_ci"
  & cmake ".." "-G" $Env:CMAKE_GENERATOR "-A" $Env:CMAKE_PLATFORM "-DBUILD_SHARED_LIBS=$ENV:BUILD_SHARED_LIBS" `
    "-DPROJECT_ENABLE_UNITTEST=ON" "-DPROJECT_ENABLE_SAMPLE=ON" "-DPROJECT_ENABLE_TOOLS=ON"
  & cmake --build . --config $Env:CONFIGURATION
  & ctest . -V -C $Env:CONFIGURATION
}
elseif ( $RUN_MODE -eq "msvc.2017.test" ) {
  New-Item -Path "build_jobs_ci" -ItemType "directory" -Force 
  Set-Location "build_jobs_ci"
  & cmake ".." "-G" $Env:CMAKE_GENERATOR "-DBUILD_SHARED_LIBS=$ENV:BUILD_SHARED_LIBS" `
    "-DPROJECT_ENABLE_UNITTEST=ON" "-DPROJECT_ENABLE_SAMPLE=ON" "-DPROJECT_ENABLE_TOOLS=ON"
  & cmake --build . --config $Env:CONFIGURATION
  & ctest . -V -C $Env:CONFIGURATION
}

Set-Location $WORK_DIR
