# Jungle SQL Processor

이 프로젝트는 완전한 SQL 엔진을 만드는 것이 주 목표가 아니라, **C 언어의 포인터와 구조체를 실제 코드에서 활용해 보며 학습하는 것**을 중심에 둔 프로젝트입니다.

`SELECT`와 `INSERT`만 최소 구현한 이유도, 제한된 범위 안에서 입력 처리, 문자열 파싱, 구조체 기반 명령 표현, 포인터 전달, 동적 메모리 관리, 파일 I/O를 집중적으로 연습하기 위해서입니다.

## 프로젝트 목적

- C 언어의 포인터와 구조체를 실제 프로그램 흐름 안에서 사용해 본다.
- 문자열 입력이 어떻게 파싱되고 실행 단계까지 전달되는지 경험한다.
- 파일 기반 저장 방식을 통해 메모리와 파일 I/O를 함께 다뤄 본다.
- AI를 활용해 빠르게 구현하되, 생성된 코드의 구조와 동작 원리를 설명할 수 있는 수준까지 이해한다.

## 왜 `SELECT`, `INSERT`만 구현했는가

이 프로젝트는 데이터베이스 기능을 넓게 구현하는 과제라기보다, C 언어 학습을 위해 적절한 난이도의 처리 흐름을 직접 만들어 보는 데 초점이 있습니다.

- `INSERT`는 입력값 검증, 컬럼 순서 맞추기, 파일 append를 연습하기 좋습니다.
- `SELECT`는 컬럼 추출, 문자열 배열 처리, 출력 버퍼 조립을 연습하기 좋습니다.
- 반면 `WHERE`, `JOIN`, `UPDATE`, `DELETE`까지 확장하면 SQL 기능 자체가 중심이 되어 포인터와 구조체 학습 목적이 흐려질 수 있습니다.

즉, 현재 프로젝트 범위는 **학습 목표에 맞춰 범위를 의도적으로 제한한 최소 구현** 입니다.

## 이 프로젝트에서 포인터와 구조체를 어떻게 사용했는가

- `SqlCommand` 구조체
  SQL 파싱 결과를 단순 문자열이 아니라 하나의 명령 객체로 묶어 전달합니다.
- 문자열 포인터와 문자열 배열 포인터
  `table_name`, `columns`, `values`, `output` 등을 통해 C에서 문자열과 문자열 목록을 어떻게 다루는지 연습합니다.
- 동적 메모리 관리
  `malloc`, `realloc`, `free`를 사용해 SQL 파싱 결과와 출력 버퍼를 직접 생성하고 해제합니다.
- 함수 간 포인터 전달
  Parser, Executor, Storage 사이에서 구조체 포인터와 출력 포인터를 주고받으며 데이터 흐름을 구성합니다.

이 프로젝트는 SQL 처리기 형태를 빌려, **포인터와 구조체를 실제 데이터 흐름 안에서 사용하는 경험**에 초점을 맞췄습니다.

## 주요 기능

- SQL 파일 입력 처리
- interactive SQL 입력 처리
- `INSERT` 지원
- `SELECT` 지원
- CSV 파일 기반 데이터 저장 및 조회
- `SELECT` 결과 표 형식 출력
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

전체 처리 흐름은 아래와 같습니다.

`main -> cli_runner -> parser -> command -> executor -> storage`

- CLI가 SQL 파일 또는 interactive 입력을 받습니다.
- Parser가 SQL을 `SqlCommand` 구조체로 변환합니다.
- Executor가 명령 종류를 보고 실행 경로를 결정합니다.
- Storage가 `data/<table>.csv` 파일을 읽거나 씁니다.

## 데이터 저장 방식

각 테이블은 CSV 파일 하나로 관리합니다.

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

- 첫 줄은 컬럼명 헤더입니다.
- 이후 줄은 데이터 행입니다.
- 테이블명 `<table>`은 `data/<table>.csv`에 대응됩니다.

`data/users.csv`는 프로그램이 바로 동작하는 모습을 보여 주기 위한 **예제 테이블 파일**입니다. 저장소를 clone한 뒤 별도 준비 없이 `SELECT`, `INSERT`를 바로 시연할 수 있어 발표와 데모에 적합합니다.

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
- 파일 입력 모드에서는 한 번에 SQL 문 1개만 처리합니다.
- interactive 모드에서도 한 줄에 SQL 문 1개를 입력합니다.

## 주요 디렉터리 구조

```text
src/            구현 코드
include/        헤더 파일
tests/          단위 테스트 / 통합 테스트
docs/           요구사항, 아키텍처, 테스트 문서
learning-docs/  학습용 문서
data/           예제 CSV 테이블 파일
```

## 빌드 방법

PowerShell에서:

```powershell
.\build.ps1
```

현재 스크립트는 아래 컴파일러 중 하나를 찾습니다.

- Visual C++ `cl`
- `gcc`

Windows에서는 Visual Studio C/C++ Build Tools 환경이 필요합니다. 기본 PowerShell을 열면 PowerShell 프로필에서 Visual Studio 개발자 환경을 자동으로 불러오도록 설정되어 있습니다.

## 테스트 실행

전체 테스트 실행:

```powershell
.\run_tests.ps1
```

## 실행 방법

### 파일 입력 모드

```powershell
.\build\sql_processor.exe input.sql
```

[input.sql](/C:/developer_folder/jungle-sql-processor/input.sql)은 `SELECT id, name FROM users;`를 담고 있는 예제 SQL 파일입니다.
[input_insert.sql](/C:/developer_folder/jungle-sql-processor/input_insert.sql)은 `INSERT` 시연용 예제 파일입니다.
[input_select_reordered.sql](/C:/developer_folder/jungle-sql-processor/input_select_reordered.sql)은 컬럼 순서 유지 출력을 보여 주는 예제 파일입니다.

예시:

```powershell
.\build\sql_processor.exe input.sql
.\build\sql_processor.exe input_select_reordered.sql
```

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

```sql
SELECT name, id FROM users;
```

```text
+--------+----+
| name   | id |
+--------+----+
| jungle | 1  |
| sql    | 2  |
+--------+----+
```

## 프로젝트에서 확인할 수 있는 학습 포인트

- SQL 기능 확장보다 포인터와 구조체 학습이 더 큰 목표라는 점
- 문자열 파싱 결과를 구조체로 표현하고 모듈 간에 전달하는 흐름
- CSV 파일 기반 저장으로 파일 I/O와 데이터 표현을 단순하게 설계한 방식
- AI를 활용해 빠르게 구현하면서도 테스트와 코드 이해를 통해 결과를 검증한 과정

## 현재 구현의 성격

이 프로젝트는 과제 요구사항에 맞춘 최소 구현입니다.

초점은 아래에 있습니다.

- 읽기 쉬운 구조
- 빠르게 이해 가능한 흐름
- 테스트 가능한 최소 기능
- SQL 처리의 전체 흐름 체험
- 포인터와 구조체를 실제 코드에 적용해 보는 경험

즉, 복잡한 DB 기능 구현보다는 **C 언어 학습과 설명 가능한 코드 이해**에 더 큰 의미가 있습니다.

## 문서 안내

- [docs/requirements.md](/C:/developer_folder/jungle-sql-processor/docs/requirements.md)
- [docs/architecture.md](/C:/developer_folder/jungle-sql-processor/docs/architecture.md)
- [docs/testing.md](/C:/developer_folder/jungle-sql-processor/docs/testing.md)
- [learning-docs/README.md](/C:/developer_folder/jungle-sql-processor/learning-docs/README.md)
