# SQL Processor 필수 구현 아키텍처 문서

## 1. 문서 목적

이 문서는 [requirements.md](C:\developer_folder\jungle-sql-processor\docs\requirements.md)를 기준으로 SQL Processor의 필수 구현 아키텍처를 정의한다.

목표는 과제 원문에 맞는 최소 구조를 유지하면서, `WHERE`를 공통 조건 표현으로 수용하고 현재는 `SELECT ... WHERE <column> = <value>`에 적용하는 설계를 제시하는 것이다.

## 2. 설계 원칙

필수 구현 아키텍처는 아래 원칙을 따른다.

- 입력, 파싱, 실행, 저장을 분리한다.
- 구현은 단순하게 유지한다.
- 파일 기반 저장 구조를 우선한다.
- 과도한 추상화는 도입하지 않는다.
- 테스트 가능한 구조를 유지한다.

## 3. 전체 구조

필수 구현은 아래 5개 모듈로 구성한다.

1. CLI
2. Parser
3. Command
4. Executor
5. Storage

테스트는 별도 디렉터리에서 관리한다.

각 모듈의 책임은 아래와 같다.

- CLI: 입력 수집, 실행 시작, 출력 및 오류 표시
- Parser: SQL 문자열과 공통 조건 표현을 내부 명령 구조로 변환
- Command: `SqlCommand` 메모리 수명 관리
- Executor: 명령 종류에 따라 저장 또는 조회 수행
- Storage: 테이블 파일 읽기/쓰기 수행

## 4. 처리 흐름

필수 구현의 표준 처리 흐름은 다음과 같다.

1. `main`이 CLI 인자를 확인한다.
2. SQL 입력 파일을 읽거나 interactive 입력을 받는다.
3. Command 모듈이 `SqlCommand`를 초기화하고 Parser가 SQL 문자열을 채운다.
4. Executor가 `SqlCommand`를 받아 적절한 Storage 동작을 호출한다.
5. Storage가 테이블 파일을 읽거나 쓰고, 필요하면 조건에 맞는 행만 선택한다.
6. Command 모듈이 사용이 끝난 `SqlCommand`를 정리한다.
7. 결과 또는 오류를 현재 터미널에 출력한다.

현재 구현에서는 `SELECT` 결과를 사람이 읽기 쉬운 표 형식으로 정리해 출력할 수 있으며, 현재 `WHERE`는 `SELECT`에만 적용하지만 내부 표현은 공통 조건 필드로 유지한다.

## 5. 모듈 설계

### 5.1 CLI

CLI는 프로그램의 진입점이다.

주요 책임:

- 실행 인자 개수 확인
- SQL 파일 경로 수집
- interactive 입력 시작
- SQL 파일 읽기 호출
- 실행 결과를 표준 출력 또는 표준 오류에 기록

예상 파일:

- `src/main.c`
- `src/cli_runner.c`
- `include/cli_runner.h`

CLI는 SQL 문법 해석이나 CSV 처리 로직을 직접 구현하지 않는다.
현재 구현은 SQL 파일 입력 모드 외에 interactive 입력 모드도 제공할 수 있다.

### 5.2 Parser

Parser는 SQL 텍스트를 내부 명령 구조로 변환한다.

지원 범위:

- `INSERT INTO <table> (<columns>) VALUES (<values>)`
- `SELECT <columns> FROM <table>`
- `SELECT <columns> FROM <table> WHERE <column> = <value>`

주요 책임:

- 명령 종류 판별
- 테이블명 추출
- 컬럼 목록 추출
- 값 목록 추출
- `WHERE` 절 존재 여부 확인
- 조건 컬럼과 조건 값 추출
- 문자열 리터럴의 바깥쪽 단일 인용부호 제거
- 입력 파일당 SQL 문 1개 처리
- 세미콜론 처리 정책 일관 유지
- 잘못된 SQL 형식 검출

Parser는 파일 시스템을 직접 다루지 않는다.
또한 `WHERE`는 내부적으로 명령 공통 조건 표현으로 파싱한다.
현재 필수 구현에서 `WHERE`를 실제로 적용하는 명령은 `SELECT`뿐이며, `SELECT *`는 지원하지 않는다.

### 5.3 Command

Command 모듈은 `SqlCommand` 구조체의 메모리 수명을 관리한다.

주요 책임:

- `sql_command_init()`로 모든 포인터를 `NULL`, 개수를 `0`으로 초기화
- `has_where`를 기본값 `0`으로 초기화
- `sql_command_free()`에서 `table_name`, `columns`, `values`뿐 아니라 `where_column`, `where_value`도 해제
- 오류 발생 경로에서도 부분적으로 채워진 `SqlCommand`를 안전하게 정리

이 구조는 현재 `SELECT`용 조건에 사용되지만, 향후 `UPDATE`/`DELETE`의 대상 행 선택에도 그대로 재사용할 수 있다.

예상 파일:

- `src/command.c`
- `include/command.h`

### 5.4 Executor

Executor는 파싱 결과를 실제 동작으로 연결한다.

주요 책임:

- `INSERT`와 `SELECT` 분기
- 대상 테이블 파일 존재 여부 확인 요청
- `INSERT`면 Storage에 레코드 추가 요청
- `SELECT`면 `WHERE` 조건 유무를 포함한 조회 요청 전달
- 결과가 없더라도 출력 형식을 유지하도록 Storage 결과를 그대로 전달
- 결과를 출력 가능한 형태로 정리

Executor는 SQL 문자열을 직접 다시 해석하지 않는다.
현재는 조건 표현을 `SELECT`에만 적용하지만, 추후 `UPDATE`/`DELETE`가 추가되면 같은 조건 필드를 기반으로 대상 행을 결정하는 분기를 확장할 수 있다.

### 5.5 Storage

Storage는 파일 기반 테이블 저장소다.

주요 책임:

- 대상 테이블 파일 열기
- 헤더 읽기
- 헤더에서 컬럼 위치 찾기
- 새 레코드 append
- 전체 레코드 읽기
- 조건 컬럼 값 비교
- 조건 일치 행만 선택
- 결과가 없는 경우 헤더와 테두리만 포함한 빈 표 생성
- 요청 컬럼만 선택해 반환
- 표 형식 출력 문자열 생성

최소 구현에서는 테이블 파일의 첫 줄 헤더를 컬럼 정의로 사용한다.
테이블명 `<table>`은 `data/<table>.csv` 파일에 매핑한다.

비교 규칙:

- 조건 비교는 대소문자를 구분하는 정확한 문자열 일치다.
- 숫자 값도 CSV에 저장된 텍스트와 정확히 일치해야 한다.
- 문자열 리터럴은 Parser가 단일 인용부호를 제거한 뒤 비교한다.

현재는 이 비교 규칙을 `SELECT` 필터링에 적용한다.
향후 `UPDATE`/`DELETE`가 추가되면 같은 비교 규칙으로 대상 행 집합을 결정한다.

### 5.6 Tests

테스트는 필수 구현 검증을 담당한다.

주요 책임:

- 파서 단위 테스트
- 저장소 단위 테스트
- CLI 기반 기능 테스트

## 6. 데이터 저장 구조

최소 구현에서는 CSV를 기본 저장 포맷으로 사용한다.

규칙:

- 각 테이블은 파일 하나로 관리한다.
- 첫 줄은 컬럼명 헤더다.
- 이후 줄은 데이터 레코드다.
- `INSERT`는 새 줄을 추가한다.
- `SELECT`는 헤더를 기준으로 필요한 컬럼을 찾는다.
- `WHERE`가 있으면 조건 컬럼 값을 먼저 비교한 뒤 일치하는 행만 선택한다.

예시:

```text
id,name
1,jungle
2,sql
```

## 7. 핵심 데이터 구조

필수 구현에서 권장하는 최소 데이터 구조는 아래와 같다.

### 7.1 SqlCommandType

```c
typedef enum SqlCommandType {
    SQL_COMMAND_INSERT = 0,
    SQL_COMMAND_SELECT = 1
} SqlCommandType;
```

### 7.2 SqlCommand

```c
typedef struct SqlCommand {
    SqlCommandType type;
    char *table_name;
    char **columns;
    size_t column_count;
    char **values;
    size_t value_count;
    int has_where;
    char *where_column;
    char *where_value;
} SqlCommand;
```

설명:

- `INSERT`는 `columns`, `values`를 모두 사용한다.
- `SELECT`는 `columns`를 사용한다.
- `WHERE`가 있는 명령은 `has_where`, `where_column`, `where_value`를 공통 조건 필드로 사용한다.
- `WHERE`가 없는 명령은 `has_where = 0`, `where_column = NULL`, `where_value = NULL`을 유지한다.

현재 필수 구현에서는 이 공통 조건 필드를 `SELECT`에만 사용한다.
향후 `UPDATE`, `DELETE`가 추가되면 같은 필드를 재사용할 수 있다.

## 8. 권장 디렉터리 구조

```text
src/
  main.c
  cli_runner.c
  parser.c
  command.c
  executor.c
  storage.c
include/
  cli_runner.h
  parser.h
  command.h
  executor.h
  storage.h
tests/
  unit/
  integration/
data/
docs/
```

설명:

- `src/`: 구현 파일
- `include/`: 헤더 파일
- `tests/unit/`: 함수 단위 테스트
- `tests/integration/`: 기능 테스트
- `data/`: 테이블 파일 저장 위치

`WHERE` 확장만으로는 새로운 계층을 추가하지 않는다. 조건 해석은 Parser가, 조건 적용은 Executor와 Storage가 기존 계층 안에서 분담한다.
이 분리는 현재 `SELECT`뿐 아니라 향후 `UPDATE`/`DELETE`에도 그대로 유지할 수 있다.

## 9. 의존성 방향

의존성은 아래 방향으로만 흐르도록 한다.

```text
CLI -> Parser
CLI -> Command
CLI -> Executor -> Storage
Tests -> 각 모듈
```

세부 원칙:

- CLI는 Storage를 직접 다루지 않는다.
- Parser는 파일 시스템에 의존하지 않는다.
- Command는 SQL 의미를 해석하지 않고 메모리 정리만 담당한다.
- Executor는 Parser가 생성한 결과를 받아 Storage를 호출한다.
- Storage는 SQL 문법을 해석하지 않는다.
- Storage는 이미 해석된 조건 정보만 사용해 행 필터링을 수행한다.

즉, 조건 표현은 특정 명령에 종속되지 않고 `SqlCommand`의 일부로 전달된다.

## 10. 오류 처리 방식

필수 구현에서는 복잡한 오류 체계 대신 기본 오류 처리를 사용한다.

최소한 아래 오류는 구분 가능해야 한다.

- 입력 파일 읽기 실패
- SQL 파싱 실패
- 대상 테이블 파일 없음
- 존재하지 않는 컬럼
- 컬럼 수와 값 수 불일치
- 잘못된 `WHERE` 문법
- `SELECT *` 사용

오류는 사용자에게 현재 터미널에서 확인 가능한 메시지로 출력한다.

## 11. 테스트 전략

### 11.1 단위 테스트

대상:

- SQL 문장 종류 판별
- `INSERT` 파싱
- `SELECT` 파싱
- `SELECT ... WHERE ...` 파싱
- `SqlCommand` 메모리 초기화/해제
- CSV 행 추가
- CSV 컬럼 조회
- 조건 기반 행 선택
- 빈 결과 집합 출력

향후 `UPDATE`/`DELETE`가 추가되면, 위 테스트 항목 중 조건 파싱과 조건 비교 관련 검증을 그대로 재사용할 수 있다.

### 11.2 기능 테스트

대상:

- SQL 파일로 `INSERT` 실행
- SQL 파일로 `SELECT` 실행
- SQL 파일로 `SELECT ... WHERE ...` 실행
- interactive 모드 실행
- 실패 케이스 확인

테스트는 실제 테이블 파일을 사용해 검증할 수 있다.

## 12. 구현 순서 권장안

권장 구현 순서는 아래와 같다.

1. `SqlCommand` 구조체 확장
2. Parser에 `WHERE` 정보 추가
3. Command 모듈의 초기화/해제 로직 갱신
4. Storage에 조건 기반 조회 추가
5. Executor에 조건 전달 로직 추가
6. CLI 구현
7. 단위 테스트 작성
8. 기능 테스트 작성

향후 `UPDATE`/`DELETE` 확장 시에는 현재 `WHERE` 공통 조건 표현을 유지한 채, 각 명령의 실행 로직만 추가하면 된다.

## 13. 아키텍처 요약

필수 구현 아키텍처의 핵심은 아래 한 줄로 요약할 수 있다.

`CLI -> Command/Parser -> Executor -> Storage -> Command`

이 구조를 유지하면 과제 원문에서 요구한 최소 기능을 빠르게 구현할 수 있다.
