param()

$ErrorActionPreference = "Stop"

& "$PSScriptRoot\build.ps1"
& "$PSScriptRoot\build\test_parser.exe"
& "$PSScriptRoot\build\test_storage.exe"
& "$PSScriptRoot\build\test_cli.exe"
