# SQL Processor 필수 구현 아키텍처 문서

## 1. 문서 목적

이 문서는 [requirements.md](C:\developer_folder\jungle-sql-processor\docs\requirements.md)를 기준으로 SQL Processor의 필수 구현 아키텍처를 정의한다.

목표는 과제 원문에 맞는 최소 구조로 빠르게 구현할 수 있는 설계를 제시하는 것이다.

## 2. 설계 원칙

필수 구현 아키텍처는 아래 원칙을 따른다.

- 입력, 파싱, 실행, 저장을 분리한다.
- 구현은 단순하게 유지한다.
- 파일 기반 저장 구조를 우선한다.
- 과도한 추상화는 도입하지 않는다.
- 테스트 가능한 구조를 유지한다.

## 3. 전체 구조

필수 구현은 아래 4개 모듈로 구성한다.

1. CLI
2. Parser
3. Executor
4. Storage

테스트는 별도 디렉터리에서 관리한다.

각 모듈의 책임은 아래와 같다.

- CLI: 입력 파일 경로 수집, 실행 시작, 출력 및 오류 표시
- Parser: SQL 문자열을 내부 명령 구조로 변환
- Executor: 명령 종류에 따라 저장 또는 조회 수행
- Storage: 테이블 파일 읽기/쓰기 수행

## 4. 처리 흐름

필수 구현의 표준 처리 흐름은 다음과 같다.

1. `main`이 CLI 인자를 확인한다.
2. SQL 입력 파일을 읽는다.
3. Parser가 SQL 문자열을 `Command` 구조체로 변환한다.
4. Executor가 `Command`를 받아 적절한 Storage 동작을 호출한다.
5. Storage가 테이블 파일을 읽거나 쓴다.
6. 결과 또는 오류를 현재 터미널에 출력한다.

## 5. 모듈 설계

### 5.1 CLI

CLI는 프로그램의 진입점이다.

주요 책임:

- 실행 인자 개수 확인
- SQL 파일 경로 수집
- SQL 파일 읽기 호출
- 실행 결과를 표준 출력 또는 표준 오류에 기록

예상 파일:

- `src/main.c`
- `src/cli_runner.c`
- `include/cli_runner.h`

CLI는 SQL 문법 해석이나 CSV 처리 로직을 직접 구현하지 않는다.

### 5.2 Parser

Parser는 SQL 텍스트를 내부 명령 구조로 변환한다.

지원 범위:

- `INSERT INTO <table> (<columns>) VALUES (<values>)`
- `SELECT <columns> FROM <table>`

주요 책임:

- 명령 종류 판별
- 테이블명 추출
- 컬럼 목록 추출
- 값 목록 추출
- 입력 파일당 SQL 문 1개 처리
- 세미콜론 처리 정책 일관 유지
- 잘못된 SQL 형식 검출

Parser는 파일 시스템을 직접 다루지 않는다.

### 5.3 Executor

Executor는 파싱 결과를 실제 동작으로 연결한다.

주요 책임:

- `INSERT`와 `SELECT` 분기
- 대상 테이블 파일 존재 여부 확인 요청
- `INSERT`면 Storage에 레코드 추가 요청
- `SELECT`면 Storage에 레코드 조회 요청
- 결과를 출력 가능한 형태로 정리

Executor는 SQL 문자열을 직접 다시 해석하지 않는다.

### 5.4 Storage

Storage는 파일 기반 테이블 저장소다.

주요 책임:

- 대상 테이블 파일 열기
- 헤더 읽기
- 헤더에서 컬럼 위치 찾기
- 새 레코드 append
- 전체 레코드 읽기
- 요청 컬럼만 선택해 반환

최소 구현에서는 테이블 파일의 첫 줄 헤더를 컬럼 정의로 사용한다.
테이블명 `<table>`은 `data/<table>.csv` 파일에 매핑한다.

### 5.5 Tests

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

예시:

```text
id,name
1,jungle
2,sql
```

## 7. 핵심 데이터 구조

필수 구현에서 권장하는 최소 데이터 구조는 아래와 같다.

### 7.1 CommandType

```c
typedef enum {
    COMMAND_INSERT,
    COMMAND_SELECT
} CommandType;
```

### 7.2 Command

```c
typedef struct {
    CommandType type;
    char *table_name;
    char **columns;
    size_t column_count;
    char **values;
    size_t value_count;
} Command;
```

설명:

- `INSERT`는 `columns`, `values`를 모두 사용한다.
- `SELECT`는 `columns`만 사용한다.

## 8. 권장 디렉터리 구조

```text
src/
  main.c
  cli_runner.c
  parser.c
  executor.c
  storage.c
include/
  cli_runner.h
  parser.h
  executor.h
  storage.h
  command.h
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

## 9. 의존성 방향

의존성은 아래 방향으로만 흐르도록 한다.

```text
CLI -> Parser
CLI -> Executor -> Storage
Tests -> 각 모듈
```

세부 원칙:

- CLI는 Storage를 직접 다루지 않는다.
- Parser는 파일 시스템에 의존하지 않는다.
- Executor는 Parser가 생성한 결과를 받아 Storage를 호출한다.
- Storage는 SQL 문법을 해석하지 않는다.

## 10. 오류 처리 방식

필수 구현에서는 복잡한 오류 체계 대신 기본 오류 처리를 사용한다.

최소한 아래 오류는 구분 가능해야 한다.

- 입력 파일 읽기 실패
- SQL 파싱 실패
- 대상 테이블 파일 없음
- 존재하지 않는 컬럼
- 컬럼 수와 값 수 불일치

오류는 사용자에게 현재 터미널에서 확인 가능한 메시지로 출력한다.

## 11. 테스트 전략

### 11.1 단위 테스트

대상:

- SQL 문장 종류 판별
- `INSERT` 파싱
- `SELECT` 파싱
- CSV 행 추가
- CSV 컬럼 조회

### 11.2 기능 테스트

대상:

- SQL 파일로 `INSERT` 실행
- SQL 파일로 `SELECT` 실행
- 실패 케이스 확인

테스트는 실제 테이블 파일을 사용해 검증할 수 있다.

## 12. 구현 순서 권장안

권장 구현 순서는 아래와 같다.

1. `Command` 구조체 정의
2. Parser 구현
3. Storage 구현
4. Executor 구현
5. CLI 구현
6. 단위 테스트 작성
7. 기능 테스트 작성

## 13. 아키텍처 요약

필수 구현 아키텍처의 핵심은 아래 한 줄로 요약할 수 있다.

`CLI -> Parser` 와 `CLI -> Executor -> Storage`

이 구조를 유지하면 과제 원문에서 요구한 최소 기능을 빠르게 구현할 수 있다.
