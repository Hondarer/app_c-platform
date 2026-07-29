# com_util コーディング規範 (特化事項)

## 概要

本書は、上位の「コーディング規範」(`docs/c-modernization-kit/coding-guideline.md`) の一般則に対して、com_util を利用するコードおよび com_util 自身に適用する特化事項をまとめます。  
章立ては上位文書の章に対応させ、com_util 固有の追記・上書き事項のみを記載します。

com_util 固有の規則、制限、遵守事項は、今後もすべて本書に集約します。

### 関連ドキュメント

- [`platform-abstraction-guideline.md`](platform-abstraction-guideline.md) - `platform.h` / `compiler.h` の共通マクロ利用規則

## エラー処理と戻り値規約

com_util の公開 API が戻り値として使用する共通結果コードの運用を示します。

### 定義ヘッダー

共通結果コードは `prod/include/com_util/base/result.h` に定義します。  
`result.h` が正であり、本節のコード一覧は参照用の写しです。

| コード | 値 | 意味 |
|---|---|---|
| `COM_UTIL_OK` | 0 | 成功 |
| `COM_UTIL_ERR_UNKNOWN` | -1 | -2 以下の分類済みコードに該当しないその他のエラー |
| `COM_UTIL_ERR_INVALID_ARGUMENT` | -2 | API 引数が不正 (NULL、負値など) |
| `COM_UTIL_ERR_UNSUPPORTED` | -3 | 現在のプラットフォームまたは状態では操作がサポートされない |
| `COM_UTIL_ERR_OUT_OF_MEMORY` | -4 | メモリを確保できない |
| `COM_UTIL_ERR_PERMISSION_DENIED` | -5 | 権限不足 |
| `COM_UTIL_ERR_TIMEOUT` | -6 | タイムアウト |
| `COM_UTIL_ERR_BUSY` | -7 | リソースがビジー状態 |
| `COM_UTIL_ERR_BUFFER_TOO_SMALL` | -8 | 出力バッファーが不足している |
| `COM_UTIL_ERR_LIMIT_EXCEEDED` | -9 | リソースの上限を超過した |
| `COM_UTIL_ERR_CORRUPT_DESCRIPTOR` | -10 | ディスクリプタが破損している |
| `COM_UTIL_ERR_DUPLICATE_DEFINITION` | -11 | 同名の定義が登録済み |
| `COM_UTIL_ERR_PARSE` | -12 | 解析エラー |
| `COM_UTIL_ERR_EOF` | -13 | 入力が EOF に達した |
| `COM_UTIL_ERR_CANCELED` | -14 | ユーザー操作 (Ctrl+C など) による中断 |

各コードの値は ABI として凍結します。  
既存の値の変更は禁止し、コードの追加は末尾 (より小さい負値) への追記のみとします。

### 判定慣用句

呼び出し側の成否判定は、コード名との比較を正とします。

```c
int rc = com_util_mmap_attach(path, access, create_size, &map);
if (rc != COM_UTIL_OK)
{
    return rc;
}
```

全エラーが負値のため `rc < 0` も等価ですが、名前比較を推奨します。  
特定のエラーを区別する場合は、`rc == COM_UTIL_ERR_TIMEOUT` のようにコード名で比較します。  
`-1` などの数値リテラルとの比較は行いません。

### 戻り値とエラー詳細の役割分担

戻り値は「分類済みの結果コード」を伝達し、OS 由来の詳細は出力引数で伝達します。

| 伝達手段 | 内容 |
|---|---|
| 戻り値 (`int`) | `COM_UTIL_OK` または負値の分類済みエラー コード |
| `int *errno_out` などの出力引数 | 生の詳細値。Linux では `errno`、Windows では `GetLastError()` の値 |

分類済みコードでは失われる詳細 (`ENOENT` と `EACCES` の区別など) が必要な API のみ、`errno_out` を提供します (`com_util_fopen`、`crt/path.h` の各関数など)。  
`errno`、`GetLastError()`、`HRESULT` などの OS エラー値を、共通結果コードとして直接返しません。

### 詳細コードの扱い

共通結果コードより細かい粒度の「詳細コード」は、モジュール固有の定義を許容します。  
com_util では argparser の `COM_UTIL_ARGPARSER_ERROR_*` (解析失敗の詳細分類) が該当します。  
詳細コードは関数の戻り値としては使用せず、取得用 API または出力引数で伝達します。

### 適用対象外

com_util は Linux / Windows のインターフェース差異を抽象化する層でもあるため、以下の API 群は元 API の戻り値規約を保存し、共通結果コードの適用対象外とします。

| 対象外の API 群 | 現行規約 | 理由 |
|---|---|---|
| CRT ラッパー (`com_util_fopen`、`com_util_fclose`、`com_util_fread`、`com_util_open`、`com_util_read`、`com_util_fseek`、`com_util_ftell` など) | `FILE *`/NULL、0/EOF、要素数、fd/-1 など元 API と同一 | 標準 C / POSIX の感覚で使えることが設計意図 |
| `com_util_strcpy` 系、`com_util_getenv` | 成功 0 / バッファー不足 `ERANGE` | CRT の `strcpy_s` 系規約に準拠 |
| `com_util_sscanf`、`com_util_utf8_to_wpath` など | 変換項目数 / 変換文字数 | 元 API の意味を保存 |
| `win32/win32.h` の UTF-8 ラッパー (`CreateFileU` など) | `HANDLE`、`BOOL` など Windows ネイティブ規約 | 元 API の差し替えとして使えることが設計意図 |
| ハンドル生成系 (`*_create` など) | 成功時ポインター / 失敗時 NULL | ポインター返却 API の慣用 |
| 値をそのまま返す関数 (getter、`com_util_timespec_cmp` など) | 値そのもの | 結果コードの概念が適用されない |
| 戻り値を持たない関数 (`*_destroy` など) | `void` | 同上 |

対象外の API 群を新設する場合は、元 API との対応と戻り値規約をヘッダーの Doxygen コメントに明記します。

### 検証

```bash
# 数値リテラル比較や三値規約の残存確認
grep -nE '(==|!=)[[:space:]]*-1\b' prod/libsrc/com_util/**/*.c

# 局所テスト
make -C app/com_util test
```

## 標準時刻型 (com_util_timespec)

### 基本ルール

`app/` 配下のコードでは、時刻の受け渡しに `struct timespec` を直接使用せず、公開型 `com_util_timespec` を使用します。  
時刻の秒部、ナノ秒部、期間、時間差の型選択は、上位「コーディング規範」の「値の意味に対応する型」に従います。

| 項目 | 内容 |
|---|---|
| 型定義 | `time_t tv_sec; int64_t tv_nsec;` (16 バイト)。Linux x86-64 の `struct timespec` とレイアウト互換 |
| native 変換 | ネイティブ `struct timespec` が必要な OS API 境界では `com_util_timespec_to_native()` / `com_util_timespec_from_native()` に集約する。キャスト・混用はしない |
| 演算 | `com_util_timespec_normalize/add/sub/cmp/add_ms/diff_ms` を使用する |

### 理由

Windows UCRT の `struct timespec` は `tv_nsec` が `long` (32bit) でレイアウトが異なります。  
共有メモリ・ダンプ・プロセス間受け渡しでレイアウト互換を保つため、公開型に統一します。

詳細は `prod/include/com_util/clock/timespec.h` の Doxygen コメントを参照してください。

### 例外事項

時刻を扱う処理にて、`com_util_timespec` とは別の時刻型を利用している場合は、互換性を確保するための意図があるため、変更前にユーザーに確認するようにしてください。

## 関数引数の const 付与と Doxygen 方向タグ

### 既存の模範例

上位「コーディング規範」の「関数引数の const 付与と Doxygen 方向タグ」にすでに沿っているヘッダー (新規実装時の参考) を示します。

- `prod/include/com_util/compress/compress.h` - データ系 `[in]` が const
- `prod/include/com_util/crypto/crypto.h` - データ系 `[in]` が const
- `prod/include/com_util/runtime/module.h` - `func_addr` が `const void *`
- `prod/include/com_util/runtime/shutdown.h` - `event` が `const com_util_shutdown_event *`
- `prod/include/com_util/sync/sync.h` の `interprocess_*_export_descriptor` - `lock` が `const ..._t *`

### 内部 lock 取得検査 (grep パターンの拡張)

上位「コーディング規範」の const 付与判定にある「内部 lock 取得検査」の grep パターンには、com_util の同期プリミティブ API を追加してください。

```bash
grep -nE '(pthread_mutex_lock|EnterCriticalSection|com_util_local_(mutex|rwlock|condvar)_)' <dir>/*.c
```
