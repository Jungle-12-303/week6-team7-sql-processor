# 테스트 문서

## 1. 문서 목적

이 문서는 현재 SQL Processor 구현에서 어떤 테스트를 작성했고, 무엇을 검증했는지 정리한다.

테스트는 아래 두 목적을 가진다.

- 구현이 요구사항대로 동작하는지 확인
- 실패해야 하는 입력이 실제로 실패하는지 확인
- 현재는 `SELECT`에만 적용되는 공통 조건 표현(`WHERE`)이 이후 확장에도 재사용 가능한지 확인

## 2. 테스트 구성

테스트는 아래 3종류로 구성된다.

1. 파서 단위 테스트
2. 저장소 단위 테스트
3. CLI 통합 테스트

`SqlCommand` 메모리 초기화/해제와 공통 조건 표현 검증은 파서 단위 테스트 범위에 포함하거나 별도 단위 테스트로 보강할 수 있다.

관련 파일:

- `tests/unit/test_parser.c`
- `tests/unit/test_storage.c`
- `tests/integration/test_cli.c`

## 3. 파서 단위 테스트

파일:

- `tests/unit/test_parser.c`

필수 검증 내용:

- `INSERT` 문 정상 파싱
- `SELECT` 문 정상 파싱
- `SELECT ... WHERE <column> = <value>` 정상 파싱
- 문자열 리터럴의 단일 인용부호 제거 확인
- 세미콜론 누락 실패
- 빈 SQL 실패
- 공백만 있는 SQL 실패
- 지원하지 않는 SQL 실패
- `INSERT` 컬럼 수와 값 수 불일치 실패
- 잘못된 `WHERE` 문법 실패
- `SELECT *` 실패
- 키워드 대소문자 혼합 입력 성공

이 테스트는 `src/parser.c`가 SQL 문자열과 공통 조건 표현을 정확히 `SqlCommand` 구조체로 바꾸는지 검증한다.

## 4. 저장소 단위 테스트

파일:

- `tests/unit/test_storage.c`

필수 검증 내용:

- CSV에 새 행 추가
- 헤더 기준 컬럼 조회
- 요청 컬럼 순서대로 결과 반환
- `WHERE` 조건이 없는 전체 조회 유지
- `WHERE` 조건이 있는 필터 조회
- 대소문자를 구분한 정확한 값 비교
- 결과가 없는 `WHERE` 조회 시 빈 표 출력
- 표 형식 `SELECT` 출력 생성
- 존재하지 않는 컬럼 조회 실패
- 존재하지 않는 테이블 파일 실패
- CSV 제약 위반 값 실패
- 손상된 CSV 행 실패

이 테스트는 `src/storage.c`가 파일 기반 저장 규칙과 공통 조건 표현의 비교 규칙, 표 형식 출력 규칙을 올바르게 처리하는지 확인한다.

## 5. CLI 통합 테스트

파일:

- `tests/integration/test_cli.c`

필수 검증 내용:

- SQL 파일로 `INSERT` 실행 성공
- SQL 파일로 `SELECT` 실행 성공
- SQL 파일로 `SELECT ... WHERE ...` 실행 성공
- interactive CLI에서 `INSERT` 실행 성공
- interactive CLI에서 `SELECT` 실행 성공
- interactive CLI에서 `SELECT ... WHERE ...` 실행 성공
- interactive CLI에서 `exit` 종료 성공
- 잘못된 SQL 파일 실패
- 존재하지 않는 입력 파일 실패
- 빈 SQL 파일 실패
- 공백만 있는 SQL 파일 실패
- 잘못된 `WHERE` 문법 SQL 실패
- `SELECT *` SQL 실패

이 테스트는 실제 사용자 관점에서 `run_cli_with_streams()`와 `run_cli_interactive_with_streams()`가 공통 조건 표현을 포함한 SQL 입력을 끝까지 올바르게 처리하는지 확인한다.

## 6. 테스트 실행 방법

### 6.1 전체 테스트 실행

PowerShell에서 아래 명령을 실행한다.

```powershell
.\run_tests.ps1
```

### 6.2 빌드만 실행

```powershell
.\build.ps1
```

참고:

- 현재 빌드 스크립트는 `cl` 또는 `gcc`를 찾는다.
- Windows에서는 Visual Studio C/C++ Build Tools 환경이 필요하다.
- 기본 PowerShell을 새로 열면 PowerShell 프로필에서 Visual Studio 개발자 셸을 자동으로 로드하도록 설정할 수 있다.

## 7. 최근 실행 결과

최근 검증 시 아래 기존 테스트가 모두 통과했다.

- `test_parser`
- `test_storage`
- `test_cli`

`WHERE`를 공통 조건 표현으로 두는 현재 문서 기준에서는 아래 케이스가 회귀 기준에 포함되어야 한다.

- `SELECT ... WHERE id = 1` 파싱 성공
- `SELECT ... WHERE name = 'jungle'` 파싱 성공
- 잘못된 `WHERE` 문법 파싱 실패
- 조건 기반 `SELECT` 결과 필터링 성공
- 빈 결과 집합 출력 형식 유지
- CLI에서 `WHERE` 포함 `SELECT` 실행 성공

향후 `UPDATE`/`DELETE`가 추가되면 위 회귀 기준 중 조건 파싱, 조건 비교, 잘못된 `WHERE` 문법 검증은 그대로 재사용할 수 있어야 한다.

## 8. 테스트 산출물

테스트 실행 중 아래 산출물이 생성될 수 있다.

- `build/`
- `tests/tmp/`
- `*.obj`
- `*.exe`

이 파일들은 `.gitignore`에 추가되어 있어 깃 추적 대상이 아니다.

## 9. 현재 테스트 범위의 의미

현재 테스트는 최소 구현 요구사항과 공통 조건 표현(`WHERE`)의 현재 적용 범위를 함께 검증하는 데 초점을 둔다.

즉, 아래를 특히 중요하게 본다.

- 성공 경로가 동작하는가
- 기본 실패 경로가 막혀 있는가
- CSV 헤더 기반 컬럼 조회가 맞는가
- 조건 기반 조회가 올바르게 필터링되는가
- `SELECT` 결과 표 형식이 일관적인가
- CLI에서 오류가 사용자에게 보이는가
- interactive CLI에서 입력과 종료가 자연스러운가

조건 표현 자체는 명령 공통 규칙으로 유지하되, 현재 테스트 적용 범위는 `SELECT` 실행 경로에 한정한다.

반면 아래는 아직 깊게 검증하지 않는다.

- 성능
- 대용량 파일 처리
- 동시성
- 복합 `WHERE` 조건
- 복잡한 SQL 문법

## 10. 향후 보강 가능한 테스트

향후에는 아래 테스트를 추가할 수 있다.

- 매우 긴 SQL 입력
- 매우 긴 CSV 행
- 비정상 인코딩 입력
- 헤더 없는 파일
- 중복 컬럼명이 있는 파일
- `WHERE` 조건의 대소문자/공백 변형 입력
- `UPDATE ... WHERE ...` 대상 행 선택 테스트
- `DELETE ... WHERE ...` 대상 행 선택 테스트
- 대량 데이터 삽입/조회 테스트
