param()

$ErrorActionPreference = "Stop"

<#
 .SYNOPSIS
  프로젝트 전체 테스트를 순서대로 실행한다.

 .DESCRIPTION
  먼저 build.ps1로 최신 바이너리를 다시 만든 뒤,
  parser, storage, cli 테스트 실행 파일을 차례대로 실행한다.
  개별 테스트가 하나라도 실패하면 PowerShell이 즉시 중단되도록 구성되어 있다.
#>
& "$PSScriptRoot\build.ps1"
& "$PSScriptRoot\build\test_parser.exe"
& "$PSScriptRoot\build\test_storage.exe"
& "$PSScriptRoot\build\test_cli.exe"
