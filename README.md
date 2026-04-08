# Jungle SQL Processor

파일 기반 CSV 저장소를 대상으로 `INSERT`, `SELECT`를 실행하는 최소 구현 SQL Processor입니다. C 언어로 작성되었고, SQL 파일 입력 모드와 interactive CLI 모드를 모두 지원합니다.

## 주요 기능

- SQL 파일 입력 처리
- interactive SQL 입력 처리
- `INSERT` 지원
- `SELECT` 지원
- CSV 파일 기반 데이터 저장/조회
- `SELECT` 결과의 표 형식 출력
- 단위 테스트 및 통합 테스트 제공

## 지원하지 않는 기능

- `CREATE TABLE`
- `UPDATE`
- `DELETE`
- `JOIN`
- `WHERE`
- 트랜잭션
- 별도 스키마 메타데이터
- 타입 검증

## 동작 방식

<br /> 계층적 구조로 만들어져 있음. 각 단계별 함수를 호출하여, storage에서 CSV 파일에 최종 접속, 결과값 반환

`main -> cli_runner -> parser -> command -> executor -> storage`

- CLI가 SQL 파일을 읽거나 interactive 입력을 받는다.
- Parser가 SQL을 `SqlCommand` 구조체로 바꾼다.
- Executor가 명령 종류를 보고 실행을 분기한다.
- Storage가 `data/<table>.csv` 파일을 읽거나 쓴다.

## 저장 형식

각 테이블은 CSV 파일 하나로 관리합니다.

예:

```text
data/users.csv
```

파일 예시:

```text
id,name
1,jungle
2,sql
```

규칙:

- 첫 줄은 컬럼명 헤더
- 이후 줄은 데이터
- 테이블명 `<table>`은 `data/<table>.csv`에 매핑

## SQL 예시

### INSERT

```sql
INSERT INTO users (id, name) VALUES (1, 'jungle');
```

### SELECT

```sql
SELECT id, name FROM users;
```

주의:

- 세미콜론(`;`)은 필수입니다.
- 파일 입력 모드는 한 번에 SQL 문 1개만 처리합니다.
- interactive 모드는 한 줄에 SQL 문 1개를 입력합니다.

## 주요 디렉터리 구조

```text
src/            구현 코드
include/        헤더 파일
tests/          단위 테스트 / 통합 테스트
docs/           요구사항, 아키텍처, 테스트 문서
learning-docs/  학습용 문서
data/           런타임 CSV 테이블 파일
```

## 빌드 방법

PowerShell에서:

```powershell
.\build.ps1
```

현재 스크립트는 아래 컴파일러 중 하나를 찾습니다.

- Visual C++ `cl`
- `gcc`

Windows에서는 Visual Studio C/C++ Build Tools 환경이 필요합니다. 기본 PowerShell을 새로 열면 PowerShell 프로필에서 Visual Studio 개발자 셸을 자동으로 로드하도록 설정했습니다.

## 테스트 실행

전체 테스트 실행:

```powershell
.\run_tests.ps1
```

## 실행 방법

### 파일 입력 모드

예:

```powershell
.\build\sql_processor.exe input.sql
```

[input.sql](C:\developer_folder\jungle-sql-processor\input.sql)은 `SELECT id, name FROM users;`를 담고 있는 예제 SQL 파일입니다.

### Interactive 모드

인자 없이 실행하면 interactive 모드로 들어갑니다.

```powershell
.\build\sql_processor.exe
```

예시:

```text
Interactive SQL mode. Type 'exit' to quit.
sql> SELECT id, name FROM users;
+----+--------+
| id | name   |
+----+--------+
| 1  | jungle |
+----+--------+
sql> exit
```

## 출력 형식

`INSERT` 성공 시에는 `INSERT OK`를 출력합니다.

`SELECT`는 요청한 컬럼 순서를 유지한 표 형식으로 출력합니다.

```text
+--------+----+
| name   | id |
+--------+----+
| jungle | 1  |
| sql    | 2  |
+--------+----+
```

## 문서 안내

- [docs/requirements.md](C:\developer_folder\jungle-sql-processor\docs\requirements.md)
- [docs/architecture.md](C:\developer_folder\jungle-sql-processor\docs\architecture.md)
- [docs/testing.md](C:\developer_folder\jungle-sql-processor\docs\testing.md)
- [learning-docs/README.md](C:\developer_folder\jungle-sql-processor\learning-docs\README.md)

## 현재 구현의 성격

이 프로젝트는 과제 요구사항에 맞춘 최소 구현입니다.

즉, 아래에 초점을 맞췄습니다.

- 읽기 쉬운 구조
- 빠르게 이해 가능한 흐름
- 테스트 가능한 최소 기능
- SQL 처리의 핵심 흐름 체험

더 복잡한 DB 기능은 이후 확장 대상으로 남겨두었습니다.
