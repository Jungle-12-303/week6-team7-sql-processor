# Jungle SQL Processor

파일 기반 CSV 저장소를 대상으로 `INSERT`, `SELECT`를 실행하는 최소 구현 SQL Processor입니다. C 언어로 작성되었고, SQL 파일을 CLI로 받아 파싱 후 실행합니다.

## 주요 기능

- SQL 파일 입력 처리
- `INSERT` 지원
- `SELECT` 지원
- CSV 파일 기반 데이터 저장/조회
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

## 동작 방식(src 폴더를 중심으로)

<br /> 계층적 구조로 만들어져 있음. 각 단계별 함수를 호출하여, storage에서 CSV 파일에 최종 접속, 결과값 반환

`main -> cli_runner -> parser -> command -> executor -> storage`

- main: 프로그램 시작점 => cli_runner.c로 제어권을 바로 넘김
- cli_ruuner: 입력을 받아 파싱과 실행을 연결 
    ```
    run_cli
    -> run_cli_with_streams
        -> read_file_to_string -> execute_sql_text (파일 모드: 경로를 명시한 파일 명령어를 읽음)
        -> run_cli_interactive_with_streams (인터랙티브 모드: 직접 타이핑한 명령어를 읽음)
            -> execute_sql_text (sql 명령어 실행)
            -> is_exit_command (종료)    
    ```
- parser: SQL명령어를 해석하여 SqlCommand 형태로 가공 (select <-> insert 분기)
    ```
    파싱 전: INSERT INTO users (id, name) VALUES (1, 'jungle');
    파싱 후: type = SQL_COMMAND_INSERT
            table_name = "users"
            columns = ["id", "name"]
            column_count = 2
            values = ["1", "jungle"]
            value_count = 2
    ```
- command: 파싱 결과를 담는 역할
- executor: INSERT/SELECT 분기
- storage: CSV파일에서 실제로 저장 및 조회 수행 


## SQL 예시

### INSERT

```sql
INSERT INTO users (id, name) VALUES (1, 'jungle');
```

### SELECT

```sql
SELECT id, name FROM users;
```

## 주요 디렉터리 구조

```text
src/            구현 코드()
include/        헤더 파일
tests/          단위 테스트 / 통합 테스트
data/           런타임 CSV 테이블 파일
```


## 테스트 실행

전체 테스트 실행:

```powershell
.\run_tests.ps1
```

## 실행 방법

예:

```
1. Developer PowerShell for VS 2022 키기
2. 해당 폴더로 경로 이동 (예시 => C:\Jungle\week6-team7-sql-processor)
3. 명령어 실행(빌드) => .\build\sql_processor.exe 
4. sql> 이 뜨면 성공!
```

## 문서 안내

- [docs/requirements.md](C:\developer_folder\jungle-sql-processor\docs\requirements.md)
- [docs/architecture.md](C:\developer_folder\jungle-sql-processor\docs\architecture.md)
- [docs/testing.md](C:\developer_folder\jungle-sql-processor\docs\testing.md)
- [learning-docs/README.md](C:\developer_folder\jungle-sql-processor\learning-docs\README.md)

## 현재 구현의 성격

이 프로젝트는 과제 요구사항에 맞춘 최소 구현입니다.

즉, 아래에 초점을 맞췄습니다.
`
- 읽기 쉬운 구조
- 빠르게 이해 가능한 흐름
- 테스트 가능한 최소 기능
- SQL 처리의 핵심 흐름 체험

더 복잡한 DB 기능은 이후 확장 대상으로 남겨두었습니다.

## 회고

각 멤버의 회고는 다음과 같습니다:

윤형민: <br />
이우진: <br />
이호준: <br />
황정연: <br />

