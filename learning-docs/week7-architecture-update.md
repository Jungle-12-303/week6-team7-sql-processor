# Week 7 B+ Tree 인덱스 아키텍처 수정안

## 1. 문서 목적

이 문서는 week6 SQL 처리기 최소 구현을 기반으로, week7 수요 코딩회 요구사항인 `B+ 트리 인덱스`를 만족시키기 위해 어떤 아키텍처 보완이 필요한지 정리한다.

목표는 아래 두 가지다.

- week6의 최소 SQL 처리기를 버리지 않고 확장 가능한 구조로 전환하기
- 이미 도입된 공통 `WHERE` 조건 표현을 바탕으로, week7 요구사항인 `자동 ID 부여`, `WHERE id = ?`, `B+ Tree 인덱스 활용`, `대용량 성능 비교`를 구현 가능한 수준으로 구체화하기

---

## 2. 기준 문서

이 수정안은 아래 문서를 기준으로 작성한다.
- `7주차 수요코딩회 문서`
- `README.md`
- `docs/architecture.md`
- `docs/testing.md`

---

## 3. 현재 week6 SQL 처리기의 범위

week6 SQL 처리기는 학습용 최소 구현이다.

현재 가능한 기능은 아래와 같다.

- SQL 파일 입력 처리
- interactive 입력 처리
- `INSERT`
- `SELECT`
- `SELECT ... WHERE <column> = <value>`
- CSV 파일 기반 저장/조회
- 표 형식 출력
- 단위 테스트와 통합 테스트

현재 구조는 아래 흐름으로 동작한다.

```text
main -> cli_runner -> parser -> command -> executor -> storage
```

현재 한계는 아래와 같다.

- `WHERE`는 현재 `SELECT`에만 적용됨
- `WHERE id = ?`도 아직 인덱스를 사용하지 않음
- 자동 ID 부여 미지원
- 인덱스 구조 없음
- 모든 조회가 사실상 CSV 기반 선형 처리
- 대용량 성능 테스트 구조 없음

즉, week6 구현은 `SQL 처리 흐름 학습용 최소 구현`으로는 충분하지만, week7 요구사항을 그대로 만족시키기에는 기능과 계층이 부족하다.

---

## 4. week7 요구사항 해석

week7 공지 기준의 핵심 요구사항은 아래와 같다.

1. `B+ Tree 인덱스` 구현
2. 레코드 추가 시 `ID 자동 부여`
3. 부여된 ID를 B+ Tree에 등록
4. `WHERE id = ?` 조회에서 인덱스 사용
5. 대규모 데이터 삽입 후 성능 테스트
6. 다른 필드 기준 조회는 선형 탐색과 비교
7. 이전 주차 SQL 처리기와 연동

이 요구사항을 코드 관점으로 바꾸면 아래 4개가 핵심이다.

- 이미 도입된 공통 `WHERE` 조건 표현을 `WHERE id = ?` 중심으로 활용해야 한다.
- 저장 시 `ID 생성 + 인덱스 등록`이 함께 일어나야 한다.
- 조회 시 `id 조건`은 인덱스 경로, 그 외는 선형 경로로 분기해야 한다.
- 대량 데이터와 성능 비교를 검증할 수 있어야 한다.

---

## 5. 아키텍처 변경 요약

week7에서는 기존 구조를 아래처럼 확장하는 것이 적절하다.

```text
main
 -> cli_runner
 -> parser
 -> command
 -> executor
    -> index_manager
       -> bptree
    -> storage
```

핵심 변화는 아래와 같다.

- `Parser`: 현재 공통 `WHERE` 조건 표현 유지, week7에서는 `id` 조건 활용을 전제로 함
- `Command`: 이미 있는 `WHERE` 조건 필드를 재사용하고, 필요 시 자동 ID 처리 연계 정보를 추가 검토
- `Executor`: `SELECT` 시 인덱스 조회와 선형 탐색 분기
- `Storage`: 현재 전체 스캔 기반 `WHERE` 조회 유지, week7에서는 자동 ID 부여와 행 참조 정보 관리 추가
- `Index Manager / B+ Tree`: `id -> row reference` 매핑 제공

중요한 점은 `Storage를 없애는 것`이 아니라 `Storage 위에 인덱스 계층을 추가하는 것`이다.

---

## 6. 권장 모듈 구조

```text
src/
  main.c
  cli_runner.c
  parser.c
  command.c
  executor.c
  storage.c
  index_manager.c
  bptree.c

include/
  cli_runner.h
  parser.h
  command.h
  executor.h
  storage.h
  index_manager.h
  bptree.h

tests/
  unit/
  integration/
  benchmark/
```

설명:

- `index_manager`: Executor와 B+ Tree 사이의 중간 계층
- `bptree`: 순수 인덱스 자료구조 구현
- `benchmark`: 대량 삽입과 조회 성능 비교용 테스트/도구

`index_manager`를 두는 이유는, Executor가 B+ Tree 내부 구현을 직접 알 필요가 없게 하기 위해서다.

---

## 7. Command 구조체 현황과 week7 활용안

현재 `SqlCommand`는 이미 공통 `WHERE` 조건 표현을 담을 수 있는 수준까지 확장되어 있다.

```c
typedef enum SqlCommandType {
    SQL_COMMAND_INSERT = 0,
    SQL_COMMAND_SELECT = 1
} SqlCommandType;

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

필드 설명:

- `has_where`: `WHERE` 절 존재 여부
- `where_column`: 예: `id`, `name`
- `where_value`: 예: `123`, `'jungle'`

현재 구현에서는 `command.c`의 `sql_command_init`, `sql_command_free`도 이미 이 필드를 초기화/해제한다.

week7에서는 이 구조를 새로 뒤집기보다 아래처럼 활용하는 편이 적절하다.

- `WHERE id = ?`는 기존 `has_where`, `where_column`, `where_value`를 그대로 사용
- `Executor`는 `where_column == "id"` 여부를 기준으로 인덱스 경로로 분기
- 자동 ID 부여는 `SqlCommand` 필드 추가보다 `Storage` 또는 별도 row result 구조로 처리하는 편이 단순함

---

## 8. Parser 수정안

### 8.1 현재 상태

현재 Parser는 아래를 지원한다.

- `INSERT INTO <table> (...) VALUES (...)`
- `SELECT <columns> FROM <table>`
- `SELECT <columns> FROM <table> WHERE <column> = <value>`

### 8.2 week7에서 필요한 최소 보강

아래 구문은 이미 파싱 가능하며, week7에서는 이 표현을 `id` 인덱스 경로로 연결해야 한다.

```sql
SELECT id, name FROM users WHERE id = 123;
SELECT name FROM users WHERE name = 'jungle';
```

### 8.3 지원 범위 제안

현재 구현과 week7 초기 범위를 함께 고려하면 아래만 유지하는 것이 현실적이다.

- 단일 `WHERE`
- 단일 비교 연산자 `=`
- `AND`, `OR` 미지원
- `WHERE`는 현재 `SELECT`에만 적용

이 정도면 week7 요구사항인 `WHERE id = ?`와 선형 탐색 비교를 위한 조건 표현 기반은 이미 갖춘 셈이다.

### 8.4 Parser 책임

- `SELECT ... FROM ...` 뒤에 `WHERE` 존재 여부 확인
- `WHERE <identifier> = <value>` 파싱
- 문자열 리터럴의 바깥쪽 단일 인용부호 제거
- `id` 조건인지 다른 컬럼 조건인지 Executor가 구분 가능하도록 `SqlCommand` 채우기

---

## 9. Storage 수정안

### 9.1 현재 상태

Storage는 아래 역할만 수행한다.

- CSV 파일 열기
- 헤더 읽기
- 컬럼 위치 찾기
- 새 행 append
- 전체 행 읽기
- 전체 스캔 기반 `WHERE` 비교
- 요청 컬럼만 추출
- 표 형식 출력
- 빈 결과 집합이면 헤더와 테두리만 있는 빈 표 출력

### 9.2 week7에서 추가해야 할 책임

- 자동 ID 생성
- 삽입된 행의 참조 정보 반환
- 현재 있는 전체 스캔 기반 `WHERE` 조회 유지
- 프로그램 시작 시 CSV에서 기존 데이터 스캔 가능

### 9.3 자동 ID 부여 전략

가장 단순한 방식은 아래 둘 중 하나다.

1. CSV를 열어 마지막 ID를 읽고 `next_id = last_id + 1`
2. 시작 시 전체 CSV를 읽어 최대 ID를 계산하고 메모리에 유지

실전 구현에서는 2번이 더 안정적이다.

이유:

- 프로그램 시작 시 인덱스 재구성과 같이 처리 가능
- `next_id`를 메모리에서 관리하기 쉬움
- 매 `INSERT`마다 파일 전체를 다시 읽지 않아도 됨

### 9.4 row reference 설계

B+ Tree가 `id -> 실제 행 위치`를 알아야 하므로 아래 중 하나를 저장해야 한다.

- CSV 행 번호
- 파일 오프셋

권장:

- 초기 구현은 `행 번호`

이유:

- 구현이 단순함
- 디버깅과 테스트가 쉬움
- 메모리 기반 B+ Tree 과제에 충분함

단점:

- 특정 행 조회 시 앞에서부터 다시 읽어야 할 수 있음

하지만 week7 목표는 디스크 기반 최적화가 아니라 `인덱스 사용 여부에 따른 구조와 성능 차이 체험`이므로, 초기 버전으로는 수용 가능하다.

---

## 10. 인덱스 계층 추가안

### 10.1 신규 계층 필요성

현재 구조에는 `id` 검색을 빠르게 하기 위한 자료구조가 전혀 없다.

`B+ Tree`를 바로 Executor가 호출하게 만들 수는 있지만, 그렇게 하면 Executor가 인덱스 세부 구현과 지나치게 결합된다.

따라서 중간 계층을 둔다.

### 10.2 권장 계층

```text
Executor -> IndexManager -> B+Tree
```

### 10.3 IndexManager 책임

- 프로그램 시작 시 CSV로부터 인덱스 재구성
- `INSERT` 후 새 ID 등록
- `WHERE id = ?` 조회 요청 처리
- B+ Tree 내부 구현을 외부에 숨김

### 10.4 B+ Tree 책임

- key 삽입
- key 검색
- 필요 시 split 처리
- 내부/리프 노드 관리

### 10.5 B+ Tree key/value 설계

- key: 자동 부여된 `id`
- value: `row_ref`

예:

```text
123 -> row 57
124 -> row 58
```

---

## 11. Executor 수정안

Executor는 week7에서 가장 중요한 분기 지점이 된다.

### 11.1 현재 상태

- `INSERT`면 `storage_append_row()` 호출
- `SELECT`면 `storage_select_rows()` 호출
- `WHERE`가 있어도 현재는 모두 `storage_select_rows()` 내부의 선형 탐색으로 처리

### 11.2 week7 목표 구조

#### INSERT

1. Executor가 INSERT 요청을 받는다.
2. Storage가 새 ID를 생성한다.
3. Storage가 CSV에 행을 append한다.
4. Storage가 `row_ref`를 돌려준다.
5. Executor가 IndexManager에 `id -> row_ref` 등록을 요청한다.
6. 결과 출력

#### SELECT

1. `WHERE` 없음
   - 기존 전체 스캔 조회
2. `WHERE id = ?`
   - 인덱스 조회 경로 사용
3. `WHERE <other_column> = ?`
   - 선형 탐색 경로 사용

이 구조가 있어야 week7 요구사항인 `인덱스 사용 조회 vs 선형 탐색 조회 비교`가 가능해진다.

---

## 12. SELECT 처리 흐름 수정안

### 12.1 인덱스 경로

```text
SELECT ... WHERE id = 123;

Parser
 -> SqlCommand(has_where=1, where_column="id", where_value="123")
Executor
 -> IndexManager.find(123)
 -> row_ref 획득
 -> Storage가 해당 row_ref의 행을 읽음
 -> 요청 컬럼만 선택
 -> 표 형식 출력
```

### 12.2 선형 탐색 경로

```text
SELECT ... WHERE name = 'jungle';

Parser
 -> SqlCommand(has_where=1, where_column="name", where_value="jungle")
Executor
 -> Storage.full_scan_select(...)
 -> 전체 CSV 스캔
 -> 조건 일치 행만 선택
 -> 표 형식 출력
```

### 12.3 핵심 원칙

- `WHERE id = ?`만 인덱스 경로 사용
- 다른 컬럼 조건은 선형 탐색 유지
- 동일 출력 포맷 유지

이렇게 해야 비교 대상이 명확하다.

---

## 13. 프로그램 시작 시 초기화 전략

week7에서는 프로그램 시작 시 아래 작업이 필요하다.

1. 테이블 CSV 읽기
2. 기존 최대 ID 계산
3. 모든 기존 레코드의 `id -> row_ref`를 인덱스에 등록

즉, 단순 CLI 시작이 아니라 `테이블 상태 + 인덱스 상태`를 복원하는 부팅 단계가 필요하다.

이를 위해 아래 중 하나를 둔다.

- `index_manager_init_from_table(table_name)`
- 또는 `storage_scan_table_for_index(...)`

권장:

- `IndexManager`가 초기화 책임을 갖고, Storage는 CSV 읽기 도우미만 제공

---

## 14. 테스트 전략 보강안

week6 테스트는 최소 SQL 처리기 검증용이다.

현재는 `SELECT ... WHERE ...`까지 검증하고 있으므로, week7에서는 아래 테스트를 추가로 보강해야 한다.

### 14.1 Parser 테스트

- `SELECT ... WHERE id = 1` 파싱 성공
- `SELECT ... WHERE name = 'x'` 파싱 성공
- 잘못된 WHERE 문법 실패
- `SELECT *` 실패

### 14.2 Storage 테스트

- 자동 ID 증가 확인
- 삽입 후 row_ref 반환 확인
- WHERE 선형 탐색 결과 확인
- `WHERE id = ?` 인덱스 경로와 선형 경로 결과 일치 확인

### 14.3 B+ Tree 단위 테스트

- 단일 삽입/검색
- 여러 key 삽입 후 검색
- split 발생 시 검색 정상 동작
- 없는 key 검색 실패

### 14.4 Integration 테스트

- INSERT 후 WHERE id 조회 성공
- INSERT 후 WHERE name 조회 성공
- 프로그램 재시작 후 인덱스 재구성 성공

### 14.5 Benchmark 테스트

- 1,000,000건 삽입
- `WHERE id = ?` 평균 조회 시간 측정
- `WHERE other_field = ?` 평균 조회 시간 측정
- 성능 차이 표 또는 로그 출력

---

## 15. 권장 구현 순서

현실적인 순서는 아래와 같다.

1. 현재 `SELECT + WHERE` 동작과 테스트 기준 고정
2. Storage에 자동 ID 부여 추가
3. B+ Tree 자료구조 구현
4. IndexManager 추가
5. INSERT 후 인덱스 등록 연결
6. `WHERE id = ?`를 인덱스 경로로 전환
7. 시작 시 인덱스 재구성 추가
8. 벤치마크와 대량 테스트 추가
9. README/아키텍처/테스트 문서 업데이트

이 순서를 권장하는 이유는, 기능 완성 경로와 성능 최적화 경로를 분리할 수 있기 때문이다.

즉:

- 먼저 `WHERE`가 동작하는 기능 버전 완성
- 그다음 `id 조회만 인덱스로 가속`

이 방식이 디버깅과 발표 준비 모두에 유리하다.

---

## 16. 핵심 리스크

### 16.1 WHERE와 인덱스를 한 번에 넣으려는 경우

기존 `WHERE` 처리 경로를 건드리면서 인덱스 분기까지 한 번에 넣으면 꼬일 가능성이 높다.

대응:

- 먼저 현재 선형 탐색 `WHERE` 동작을 회귀 기준으로 고정

### 16.2 자동 ID 정책이 늦게 정해지는 경우

INSERT, CSV 포맷, 인덱스 key가 모두 흔들린다.

대응:

- 초기에 `id는 시스템 자동 부여`를 고정

### 16.3 row reference 설계가 불분명한 경우

B+ Tree는 구현돼도 실제 레코드를 못 찾을 수 있다.

대응:

- 초기에는 `row number` 방식으로 단순화

### 16.4 성능 테스트를 마지막에 붙이는 경우

대량 데이터 생성과 벤치마크 코드가 급하게 붙어 발표 품질이 떨어질 수 있다.

대응:

- 기능 구현 중간부터 데이터 생성 스크립트와 시간 측정 도구를 같이 준비

---

## 17. 최소 성공 기준

week7에서 최소 성공으로 볼 수 있는 상태는 아래와 같다.

- `INSERT` 시 ID가 자동 부여된다.
- 기존 공통 `WHERE` 조건 표현이 유지된다.
- 삽입된 ID가 B+ Tree에 등록된다.
- `WHERE id = ?`는 인덱스를 사용한다.
- 다른 필드 조건 조회는 선형 탐색으로 동작한다.
- 대량 데이터 삽입 후 두 조회 방식의 시간 차이를 시연할 수 있다.
- README와 테스트 문서로 핵심 로직을 설명할 수 있다.

---

## 18. 결론

week6 SQL 처리기는 week7의 기반으로는 충분하다.  
현재는 `WHERE`까지 포함한 최소 흐름이 이미 구현되어 있지만, week7에서 필요한 `자동 ID`, `id 조건 인덱스 경로`, `인덱스 계층`, `대량 성능 검증`은 아직 빠져 있다.

따라서 week7에서는 기존 구조를 버리기보다 아래 방향으로 확장하는 것이 적절하다.

- Parser는 이미 있는 공통 `WHERE` 조건 표현을 유지
- Command는 이미 있는 조건 필드를 인덱스 분기에 재사용
- Executor는 인덱스 경로와 선형 경로를 분기
- Storage는 자동 ID와 행 참조를 지원
- 신규 `IndexManager + B+ Tree` 계층 추가

이 구조면 week6 코드의 장점인 단순한 흐름을 유지하면서도, week7 요구사항을 구현 가능한 수준으로 발전시킬 수 있다.
