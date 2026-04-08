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

전체 흐름은 아래와 같습니다.

`CLI -> Parser -> Executor -> Storage`

설명:

- CLI가 SQL 파일을 읽는다.
- Parser가 SQL을 `SqlCommand` 구조체로 바꾼다.
- Executor가 명령 종류를 보고 실행을 분기한다.
- Storage가 `data/<table>.csv` 파일을 읽거나 쓴다.


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

```1. Developer PowerShell for VS 2022 키기
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

윤형민:
이우진:
이호준:
황정연:

