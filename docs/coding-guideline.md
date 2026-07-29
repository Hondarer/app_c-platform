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

定義は課題別の帯に分けて並べ、各帯に将来の追加用の余白を設けています。

| 帯 | コード | 値 | 意味 |
|---|---|---|---|
| 分類不能 | `COM_UTIL_OK` | 0 | 成功 |
| | `COM_UTIL_ERR_UNKNOWN` | -1 | -2 以下の分類済みコードに該当しないその他のエラー |
| 引数・状態・権限<br> (-2 〜 -9) | `COM_UTIL_ERR_INVALID_ARGUMENT` | -2 | API 引数が不正 (NULL、負値など) |
| | `COM_UTIL_ERR_UNSUPPORTED` | -3 | 現在のプラットフォームまたは状態では操作がサポートされない |
| | `COM_UTIL_ERR_PERMISSION_DENIED` | -4 | 権限不足 |
| | `COM_UTIL_ERR_DUPLICATE_DEFINITION` | -5 | 同名の定義が登録済み |
| リソース・バッファー<br> (-10 〜 -19) | `COM_UTIL_ERR_OUT_OF_MEMORY` | -10 | メモリを確保できない |
| | `COM_UTIL_ERR_BUSY` | -11 | リソースがビジー状態 |
| | `COM_UTIL_ERR_TIMEOUT` | -12 | タイムアウト |
| | `COM_UTIL_ERR_LIMIT_EXCEEDED` | -13 | リソースの上限を超過した |
| | `COM_UTIL_ERR_BUFFER_TOO_SMALL` | -14 | 出力バッファーが不足している |
| | `COM_UTIL_ERR_CORRUPT_DESCRIPTOR` | -15 | ディスクリプタが破損している |
| 入力解析<br> (-20 〜 -39) | `COM_UTIL_ERR_UNKNOWN_OPTION` | -20 | 未登録のオプションが指定された |
| | `COM_UTIL_ERR_MISSING_VALUE` | -21 | 値を要する項目に値が指定されていない |
| | `COM_UTIL_ERR_UNEXPECTED_VALUE` | -22 | 値を取らない項目に値が指定された |
| | `COM_UTIL_ERR_INVALID_INTEGER` | -23 | 整数値として解釈できない |
| | `COM_UTIL_ERR_OUT_OF_RANGE` | -24 | 値が表現可能な範囲を超えている |
| | `COM_UTIL_ERR_MISSING_REQUIRED` | -25 | 必須の項目が指定されていない |
| | `COM_UTIL_ERR_DUPLICATE_OPTION` | -26 | 単数指定の項目が複数回指定された |
| | `COM_UTIL_ERR_TOO_MANY_ARGUMENTS` | -27 | 引数の個数が受入数を超えている |
| | `COM_UTIL_ERR_TOO_MANY_OCCURRENCES` | -28 | 同一項目の出現回数が容量を超えている |
| 制御<br> (-40 〜) | `COM_UTIL_ERR_EOF` | -40 | 入力が EOF に達した |
| | `COM_UTIL_ERR_CANCELED` | -41 | ユーザー操作 (Ctrl+C など) による中断 |

帯は定義の整理と追加位置を示すためのものであり、範囲判定による分類は API の契約に含めません。  
`rc <= -20` のような範囲比較で種別を判定せず、個々のコード名との比較を使用します。

各コードの値は ABI として凍結します。  
既存の値の変更は禁止し、コードの追加は該当する帯の余白への追記のみとします。  
値は `test/src/libcom_utilTest/base/resultTest/` の `static_assert` で固定しており、変更するとコンパイル時に検出されます。

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

### 命名

エラー コードの名称は `COM_UTIL_ERR_*` に統一します。  
`ERROR` という略記は使用しません。

### 詳細分類の扱い

共通結果コードより細かい粒度の分類が必要な場合も、`result.h` へコードを追加して 1 系統に集約します。  
モジュール固有のコード体系を別に設けません。

そのため `result.h` は、粗い分類 (`COM_UTIL_ERR_INVALID_ARGUMENT` など) と細かい分類 (`COM_UTIL_ERR_UNKNOWN_OPTION` など) の両方を含みます。  
細かい分類のコードも通常の結果コードであり、関数の戻り値として返せます。  
argparser の `_com_util_argparser_parse()` は解析エラーの種別に対応するコードを直接返し、`_com_util_argparser_get_error()` はその種別を後から再取得する用途で提供しています。

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

なお、`com_util_getenv` は上表の「成功 0 / バッファー不足 `ERANGE`」に加えて「未設定 -1」を返す三値規約であり、本規約への適合は「既知の逸脱と移行課題」に整理しています。

### 検証

```bash
# 数値リテラル比較や三値規約の残存確認
grep -nE '(==|!=)[[:space:]]*-1\b' prod/libsrc/com_util/**/*.c

# 局所テスト
make -C app/com_util test
```

## CRT ラッパーの適用範囲

com_util が CRT / POSIX 関数のラッパー (`com_util_snprintf`、`com_util_fopen` など) を提供している関数については、`app/` 配下のすべてのコードでラッパーを使用します。
com_util 自身の実装 (`prod/libsrc/`) も対象に含みます (`com_util_strcpy`、`com_util_stat`、`com_util_fopen`、`com_util_sscanf` などの既存の使用例に従います)。

例外は、そのラッパー自身の実装だけです。
`com_util_vsnprintf` の実装が `vsnprintf` を呼ぶような、ラッパーが元関数へ委譲する箇所は元関数を直接呼び出します。

ラッパーを提供していない関数 (`memset`、`strlen` など) はこの規約の対象外であり、元関数をそのまま使用します。
`prod/libsrc/com_util/trace/backends/etw/trace_etw_session.c` の `zero_bytes()` が `memset` を使わず自前実装しているのは、testfw が libc の `memset` を include_override のマクロで差し替えるためであり、com_util のラッパー層とは別の理由によるものです。

`mock_com_util` は `com_util_*` を weak シンボルで差し替えるため、実装内部の呼び出しもモックの対象になります。
これは既存のラッパー使用箇所でも同様であり、モック未設定時は実関数へ委譲されます。

## API 命名規約

com_util の公開 API 名に適用する規則を示します。
既存 API の名前は ABI として凍結し、本規約への適合を目的としたリネームは行いません。
本規約は新設 API と、移行を伴う変更の際の改名先に適用します。

### 基本形

公開 API は `com_util_<カテゴリ名詞>_<動詞または属性>` の順序を正とします。

```c
com_util_file_get_size(...)     /* file カテゴリの getter */
com_util_tracer_set_name(...)   /* tracer カテゴリの setter */
com_util_path_dirname(...)      /* path カテゴリの変換 */
```

カテゴリ名詞を持たない横断的な API (`com_util_sleep_ms` など) に限り、動詞先行を許容します。
既存の `com_util_get_temp_dir`、`com_util_get_monotonic_ms`、`com_util_normalize_path_sep`、`com_util_paths_equal` は本規約に先行するため凍結対象です。

### 生成と破棄の動詞対

ハンドルを生成・破棄する API の新設時は、`*_create` / `*_dispose` の対を正とします。
既存 API の破棄動詞 (`*_destroy`、`*_detach`、`*_close`、`*_stop`、`*_release`) は凍結し、同一ハンドル型の中では既存の動詞に合わせます (例: sync カテゴリへの追加は `*_destroy` に合わせる)。

プロセス ライフサイクルで常に有効な既定インスタンスを明示的に初期化する API は `*_init` とし、破棄 API を対にしません (`com_util_console_init` の `_dispose` は終了時の状態復元であり、インスタンス破棄ではありません)。
`com_util_argparser_init` が該当します。既定パーサーはライブラリが所有し、初期化後はプロセス終了まで常に有効であり、利用側による破棄を必要としない設計です。

### アンダースコア前置きの公開シンボル

`_com_util_` 前置きの公開シンボルは、直接呼び出しを想定しないシンボルに限定します。
許容する用途は以下の 2 種のみとします。

| 用途 | 例 |
|---|---|
| 呼び出し元情報 (`__FILE__` / `__LINE__`) などを補うマクロの実体 | `_com_util_tracer_write` |
| テスト専用フック (`_for_test` サフィックスを併用) | `_com_util_shutdown_invoke_for_test` |

明示ハンドル版と既定ハンドル版の区別 (argparser の `_com_util_argparser_*` / `com_util_argparser_*`) には今後使用しません。
既存の argparser API は凍結対象です。

### typedef の規則 (上位規範への追記)

typedef struct に `_t` サフィックスを付けない規則は上位規範のとおりです。
enum と関数ポインターの typedef に付く `_t` (`com_util_trace_level_t` など) は現状を追認し、許容します。
新設の typedef enum はタグ付き (`typedef enum name { ... } name;`) を正とし、無名 enum の typedef は作成しません。

## 引数順序規約

com_util の公開 API の引数順序は、API の性格に応じて以下の 3 規則に従います。

### 変換・整形系 (COM_UTIL_OK 系)

入力を出力バッファーへ変換・整形する API は、CRT の `strcpy_s` 系に合わせて出力バッファーを先頭に置きます。

```c
戻り値 関数名(out, out_size[, errno_out], 入力...);
```

```c
com_util_strcpy(dest, dest_size, src);
com_util_path_dirname(path_out, path_size, errno_out, path);
com_util_gmtime(utc_tm, timep);
com_util_stat(buf, path);
```

`com_util_stat` が POSIX の `stat(path, buf)` と逆順であるのは、本規約 (出力先頭) によるものです。
`errno_out` を提供する場合は、出力バッファーとサイズの直後に置きます。

### ハンドル・操作系 (COM_UTIL_OK 系)

ハンドルまたは操作対象を先頭に置き、`*_out` の出力引数は末尾に置きます。

```c
com_util_file_get_size(file, size_out);
com_util_paths_equal(lhs, rhs, equal_out, errno_out);
com_util_elevated_process_run_with_result(arguments, exit_code, handled, result_message, result_message_size);
```

### 適用対象外 API と _fmt 系

「エラー処理と戻り値規約」の適用対象外 API は、元 API の引数順を保存し、追加の出力引数 (`errno_out` など) は末尾に付加します。

```c
com_util_fopen(path, modes, errno_out);        /* fopen(path, modes) + errno_out */
com_util_fopen_temp(prefix, modes, path_out, path_size, errno_out);
```

`_fmt` 系はパス引数を書式で組み立てる派生 API であり、基底 API からパス引数を除いた残りの引数順を維持し、末尾に `format` と可変長引数を置きます。
`v*_fmt` は可変長引数を `va_list args` に置き換えます。

```c
com_util_open(path, flags, mode);
com_util_open_fmt(flags, mode, format, ...);
com_util_vopen_fmt(flags, mode, format, args);
```

## 解消済みの逸脱

本規約および上位規範に対する既存公開 API の逸脱として整理していた項目は、すべて解消済みです。
旧シグネチャからの移行手順は [`api-consistency-migration.md`](api-consistency-migration.md) を参照してください。

| API | 逸脱内容 | 解消結果 |
|---|---|---|
| `com_util_getenv` | 0 / -1 (未設定) / `ERANGE` の三値。上位規範の三値禁止に抵触 | 設定有無を `int *exists_out` へ分離し、戻り値を 0 / `EINVAL` / `ERANGE` の二値系へ変更 |
| argparser の既定ハンドル版ラッパー (`com_util_argparser_register_*` など) | `void` 戻りで登録エラーを破棄。明示ハンドル版と成否可視性が異なる | 15 関数を `int` 戻りへ変更し、明示ハンドル版の結果コードを転送 |
| `com_util_pinned_prompt_write` | 引数不正時に 0 を返し、正常な 0 バイト書き込みと区別できない | 結果コード戻り + `size_t *written_out` へ変更 |
| `com_util_etw_session_start` | ハンドル戻りと `int *out_status` を併用し、他の生成系 (NULL 返却のみ) と失敗通知方式が異なる | 結果コード戻り + `com_util_etw_session **session_out` へ変更 |
| `com_util_process_options_t` | typedef struct への `_t` 別名で、上位規範の `_t` 禁止に抵触 | `com_util_process_options` へ統一。同種の `com_util_process_stdio_t` も `com_util_process_stdio` へ統一 |

`com_util_argparser_init` は、既定インスタンスを初期化する `*_init` として本規約に適合するため、逸脱には該当しません。

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
