# SQL Processor 학습 가이드

이 디렉터리는 현재 구현된 SQL Processor를 빠르게 이해하기 위한 학습용 문서를 모아둔 곳입니다.

처음 읽는 순서는 아래를 권장합니다.

1. `system-overview.md`
2. `request-flow.md`
3. `tests-guide.md`

## 이 시스템이 하는 일

이 시스템은 아주 단순한 SQL 처리기입니다.

- SQL 파일을 읽는다.
- SQL 문을 파싱한다.
- `INSERT`면 CSV 파일에 데이터를 추가한다.
- `SELECT`면 CSV 파일에서 데이터를 읽어 출력한다.

지원 범위는 의도적으로 작게 잡혀 있습니다.

- `INSERT`
- `SELECT`
- 파일 기반 저장
- CSV 헤더 기반 컬럼 조회

지원하지 않는 기능도 많습니다.

- `UPDATE`
- `DELETE`
- `JOIN`
- `WHERE`
- 별도 스키마 파일
- 타입 검증

## 가장 먼저 보면 좋은 파일

- `src/main.c`
- `src/cli_runner.c`
- `src/parser.c`
- `src/executor.c`
- `src/storage.c`

이 다섯 파일이 실제 동작의 거의 전부입니다.

## 추천 학습 방법

### 1. 큰 흐름 먼저 보기

`main -> cli_runner -> parser -> executor -> storage`

이 흐름만 먼저 잡아도 시스템이 어떻게 움직이는지 절반 이상 이해한 것입니다.

### 2. 성공 케이스 먼저 보기

아래 SQL 하나를 머릿속에 두고 따라가면 이해가 쉽습니다.

```sql
INSERT INTO users (id, name) VALUES (1, 'jungle');
```

그리고 다음 SQL을 이어서 보면 됩니다.

```sql
SELECT id, name FROM users;
```

### 3. 테스트로 검증 포인트 보기

테스트 파일을 보면 개발자가 무엇을 중요하게 생각했는지 바로 알 수 있습니다.

- `tests/unit/test_parser.c`
- `tests/unit/test_storage.c`
- `tests/integration/test_cli.c`

## 함께 보면 좋은 문서

- `docs/requirements.md`
- `docs/architecture.md`

위 문서는 이 시스템이 왜 이렇게 만들어졌는지 배경을 설명하고, 이 디렉터리 문서는 실제 구현을 어떻게 읽으면 좋은지 설명합니다.
