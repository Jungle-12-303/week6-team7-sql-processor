param()

$ErrorActionPreference = "Stop"

function Get-Compiler {
    if (Get-Command cl -ErrorAction SilentlyContinue) {
        return "cl"
    }

    if (Get-Command gcc -ErrorAction SilentlyContinue) {
        return "gcc"
    }

    throw "No supported C compiler was found in PATH. Install Visual C++ Build Tools or GCC."
}

function Ensure-Directory([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

$compiler = Get-Compiler
Ensure-Directory "build"

$appSources = @(
    "src\main.c",
    "src\cli_runner.c",
    "src\parser.c",
    "src\executor.c",
    "src\storage.c",
    "src\command.c"
)

if ($compiler -eq "cl") {
    $commonArgs = @("/nologo", "/W4", "/D_CRT_SECURE_NO_WARNINGS", "/Iinclude")

    & cl @commonArgs "/Febuild\sql_processor.exe" @appSources
    & cl @commonArgs "/Febuild\test_parser.exe" "tests\unit\test_parser.c" "src\parser.c" "src\command.c"
    & cl @commonArgs "/Febuild\test_storage.exe" "tests\unit\test_storage.c" "src\storage.c" "src\command.c"
    & cl @commonArgs "/Febuild\test_cli.exe" "tests\integration\test_cli.c" "src\cli_runner.c" "src\parser.c" "src\executor.c" "src\storage.c" "src\command.c"
} else {
    $commonArgs = @("-std=c11", "-Wall", "-Wextra", "-pedantic", "-Iinclude")

    & gcc @commonArgs -o "build\sql_processor.exe" @appSources
    & gcc @commonArgs -o "build\test_parser.exe" "tests\unit\test_parser.c" "src\parser.c" "src\command.c"
    & gcc @commonArgs -o "build\test_storage.exe" "tests\unit\test_storage.c" "src\storage.c" "src\command.c"
    & gcc @commonArgs -o "build\test_cli.exe" "tests\integration\test_cli.c" "src\cli_runner.c" "src\parser.c" "src\executor.c" "src\storage.c" "src\command.c"
}
