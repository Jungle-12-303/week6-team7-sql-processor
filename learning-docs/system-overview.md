# 시스템 개요

## 1. 한 줄 설명

이 시스템은 SQL 파일을 받아 `INSERT`와 `SELECT`를 CSV 파일에 대해 실행하는 최소 구현 SQL Processor입니다.

## 2. 핵심 구조

구조는 아주 단순합니다.

`CLI -> Parser -> Executor -> Storage`

각 모듈의 역할은 아래와 같습니다.

### CLI

역할:

- 사용자가 넘긴 SQL 파일 경로를 받는다.
- 파일 내용을 읽는다.
- 파싱과 실행을 시작한다.
- 결과를 화면에 출력한다.

관련 파일:

- `src/main.c`
- `src/cli_runner.c`
- `include/cli_runner.h`

### Parser

역할:

- SQL 문자열을 해석한다.
- `INSERT`인지 `SELECT`인지 판단한다.
- 테이블명, 컬럼명, 값 목록을 뽑아낸다.

관련 파일:

- `src/parser.c`
- `include/parser.h`

### Executor

역할:

- 파싱 결과를 실제 동작으로 연결한다.
- `INSERT`와 `SELECT`를 분기한다.
- Storage를 호출한다.
- 최종 출력 문자열을 만든다.

관련 파일:

- `src/executor.c`
- `include/executor.h`

### Storage

역할:

- `data/<table>.csv` 파일을 읽는다.
- 헤더를 기준으로 컬럼 위치를 찾는다.
- 레코드를 추가하거나 조회한다.

관련 파일:

- `src/storage.c`
- `include/storage.h`

## 3. 데이터 저장 방식

각 테이블은 CSV 파일 하나로 관리됩니다.

예시:

```text
data/users.csv
```

파일 내용 예시:

```text
id,name
1,jungle
2,sql
```

규칙:

- 첫 줄은 헤더
- 이후 줄은 데이터
- 헤더를 컬럼 정의처럼 사용

즉, 별도 스키마 파일은 없습니다.

## 4. Command 구조체가 중요한 이유

이 시스템에서 Parser와 Executor를 연결하는 핵심은 `SqlCommand` 구조체입니다.

대략 이런 정보를 담습니다.

- 명령 종류
- 테이블명
- 컬럼 목록
- 값 목록

이 구조 덕분에 Parser는 “해석”만 하고, Executor는 “실행”만 할 수 있습니다.

## 5. 왜 이렇게 단순하게 만들었나

이 프로젝트는 학습과 구현 속도를 같이 잡아야 하는 과제입니다.

그래서 아래는 일부러 하지 않았습니다.

- 별도 스키마 메타데이터
- 타입 시스템
- SQL 전체 문법 지원
- 복잡한 계층 구조

대신 아래를 분명하게 남겼습니다.

- 입력
- 파싱
- 실행
- 저장
- 테스트

이 다섯 흐름이 드러나는 것이 현재 구현의 핵심입니다.
