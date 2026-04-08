param()

$ErrorActionPreference = "Stop"

<#
 .SYNOPSIS
  프로젝트 실행 파일과 테스트 실행 파일을 빌드한다.

 .DESCRIPTION
  현재 PATH에서 사용할 수 있는 C 컴파일러를 찾은 뒤,
  앱 본체와 테스트 바이너리를 한 번에 생성한다.
  Windows MSVC와 GCC 양쪽을 모두 지원하도록 분기되어 있다.
#>
function Get-Compiler {
    if (Get-Command cl -ErrorAction SilentlyContinue) {
        return "cl"
    }

    if (Get-Command gcc -ErrorAction SilentlyContinue) {
        return "gcc"
    }

    throw "No supported C compiler was found in PATH. Install Visual C++ Build Tools or GCC."
}

<#
 .SYNOPSIS
  지정한 디렉터리가 없으면 생성한다.

 .DESCRIPTION
  빌드 산출물을 저장할 `build/` 디렉터리를 준비할 때 사용한다.
  이미 디렉터리가 있으면 아무 작업도 하지 않는다.
#>
function Initialize-Directory([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

# 현재 셸에서 사용할 컴파일러를 고르고 빌드 출력 디렉터리를 준비한다.
$compiler = Get-Compiler
Initialize-Directory "build"

# 애플리케이션 실행 파일에 포함될 메인 소스 목록이다.
$appSources = @(
    "src\main.c",
    "src\cli_runner.c",
    "src\parser.c",
    "src\executor.c",
    "src\storage.c",
    "src\command.c"
)

if ($compiler -eq "cl") {
    # MSVC 빌드 옵션: 경고 활성화, UTF-8 소스 해석, CRT 경고 억제, 헤더 경로 지정
    $commonArgs = @("/nologo", "/W4", "/utf-8", "/D_CRT_SECURE_NO_WARNINGS", "/Iinclude")

    & cl @commonArgs "/Febuild\sql_processor.exe" @appSources
    & cl @commonArgs "/Febuild\test_parser.exe" "tests\unit\test_parser.c" "src\parser.c" "src\command.c"
    & cl @commonArgs "/Febuild\test_storage.exe" "tests\unit\test_storage.c" "src\storage.c" "src\command.c"
    & cl @commonArgs "/Febuild\test_cli.exe" "tests\integration\test_cli.c" "src\cli_runner.c" "src\parser.c" "src\executor.c" "src\storage.c" "src\command.c"
} else {
    # GCC 빌드 옵션: C11 표준과 일반적인 경고 옵션을 사용한다.
    $commonArgs = @("-std=c11", "-Wall", "-Wextra", "-pedantic", "-Iinclude")

    & gcc @commonArgs -o "build\sql_processor.exe" @appSources
    & gcc @commonArgs -o "build\test_parser.exe" "tests\unit\test_parser.c" "src\parser.c" "src\command.c"
    & gcc @commonArgs -o "build\test_storage.exe" "tests\unit\test_storage.c" "src\storage.c" "src\command.c"
    & gcc @commonArgs -o "build\test_cli.exe" "tests\integration\test_cli.c" "src\cli_runner.c" "src\parser.c" "src\executor.c" "src\storage.c" "src\command.c"
}
