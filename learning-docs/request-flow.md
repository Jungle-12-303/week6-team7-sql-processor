# 요청 흐름 따라가기

## 1. `INSERT` 흐름

예시 SQL:

```sql
INSERT INTO users (id, name) VALUES (1, 'jungle');
```

### 단계 1. 프로그램 시작

`src/main.c`는 `run_cli()`를 호출합니다.

즉, 진짜 시작점은 `src/cli_runner.c`입니다.

### 단계 2. SQL 파일 읽기

`run_cli_with_streams()` 안에서 `read_file_to_string()`이 SQL 파일 전체를 문자열로 읽습니다.

여기서 실패하면 바로 오류를 출력하고 종료합니다.

### 단계 3. SQL 파싱

`parse_sql()`이 호출됩니다.

Parser는 아래를 검사합니다.

- 문장 끝 세미콜론이 있는가
- SQL이 한 문장만 있는가
- `INSERT` 또는 `SELECT`인가
- 문법이 맞는가

`INSERT`라면 아래 정보를 뽑아냅니다.

- 테이블명 `users`
- 컬럼 목록 `id`, `name`
- 값 목록 `1`, `jungle`

### 단계 4. 실행

`execute_command()`가 호출됩니다.

명령이 `INSERT`면 `storage_append_row()`를 부릅니다.

### 단계 5. 파일 저장

Storage는 `data/users.csv`를 엽니다.

그 다음 헤더를 읽고:

- 헤더 컬럼 순서 확인
- 입력 컬럼이 모두 있는지 확인
- 값을 헤더 순서에 맞게 재배열

마지막으로 새 줄을 파일 끝에 추가합니다.

예를 들어 파일이 이랬다면:

```text
id,name
```

실행 후 이렇게 됩니다.

```text
id,name
1,jungle
```

### 단계 6. 결과 출력

성공하면 `INSERT OK` 문자열을 만들어 stdout으로 보냅니다.

## 2. `SELECT` 흐름

예시 SQL:

```sql
SELECT id, name FROM users;
```

앞부분은 `INSERT`와 같습니다.

- CLI가 파일 읽기
- Parser가 SQL 해석
- Executor가 실행 분기

차이는 Storage에서 생깁니다.

### 단계 1. 헤더 읽기

`data/users.csv`의 첫 줄을 읽습니다.

예:

```text
id,name
```

### 단계 2. 요청 컬럼 위치 찾기

`SELECT id, name`이면 헤더에서 `id`, `name` 위치를 찾습니다.

이 위치 정보로 각 행에서 필요한 값만 꺼낼 수 있습니다.

### 단계 3. 모든 데이터 행 읽기

예를 들어 파일이 아래와 같다면:

```text
id,name
1,jungle
2,sql
```

출력은 아래처럼 만들어집니다.

```text
id,name
1,jungle
2,sql
```

### 단계 4. 컬럼 순서 바꾸기

만약 SQL이 아래라면:

```sql
SELECT name, id FROM users;
```

출력은 이렇게 바뀝니다.

```text
name,id
jungle,1
sql,2
```

즉, 출력 순서는 파일 순서가 아니라 요청 순서를 따릅니다.

## 3. 실패 흐름은 어디서 끊기나

### CLI 단계에서 실패

- 인자 개수가 잘못됨
- SQL 파일을 열 수 없음

### Parser 단계에서 실패

- 세미콜론 없음
- 빈 SQL
- 지원하지 않는 SQL
- 문법 오류
- 컬럼 수와 값 수 불일치

### Storage 단계에서 실패

- 테이블 파일 없음
- 헤더에 없는 컬럼 요청
- CSV 행이 손상됨
- CSV에 넣을 수 없는 값 사용

이렇게 실패 지점을 나눠 보면 디버깅도 쉬워집니다.
