# com_util コーディング規範 (特化事項)

## 概要

本書は、上位の「コーディング規範」(`docs/general/coding-guideline.md`) の一般則に対して、com_util を利用するコードおよび com_util 自身に適用する特化事項をまとめます。  
章立ては上位文書の章に対応させ、com_util 固有の追記・上書き事項のみを記載します。

com_util 固有の規則、制限、遵守事項は、今後もすべて本書に集約します。

### 関連ドキュメント

- [`platform-abstraction-guideline.md`](platform-abstraction-guideline.md) - `platform.h` / `compiler.h` の共通マクロ利用規則
- [`error-detail-migration.md`](error-detail-migration.md) - 生の OS エラー値から `com_util_error` への移行手順

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
| | `COM_UTIL_ERR_INVALID_PATTERN` | -29 | 正規表現パターンの構文が不正である |
| | `COM_UTIL_ERR_INVALID_ENCODING` | -30 | 文字列が UTF-8 として不正である |
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

戻り値は「分類済みの結果コード」を伝達し、OS 由来の詳細はドメイン付きの `com_util_error` で伝達します。

| 伝達手段 | 内容 |
|---|---|
| 戻り値 (`int`) | `COM_UTIL_OK` または負値の分類済みエラー コード |
| `com_util_error *detail_out` 出力引数 | OS エラーのドメイン、共通結果コード、生の詳細値 |
| スレッド ローカルの直前値 | `com_util_error_get_last()` で取得する、直前の対応 API と同じ詳細 |

分類済みコードでは失われる詳細 (`ENOENT` と `EACCES` の区別など) が必要な API は、`detail_out` を提供します (`com_util_fopen`、`crt/path.h` の各関数など)。  
対応 API は失敗時に出力引数とスレッド ローカルの直前値へ同じ詳細を記録し、成功時は両方をクリアします。  
`errno`、`GetLastError()`、`HRESULT` などの OS エラー値を、共通結果コードとして直接返しません。

### 命名

エラー コードの名称は `COM_UTIL_ERR_*` に統一します。  
`ERROR` という略記は使用しません。

### 詳細分類の扱い

操作結果として呼び出し元の制御フローを分岐させる分類は、`result.h` の `COM_UTIL_ERR_*` へ追加します。  
`COM_UTIL_ERR_*` は関数の戻り値として使用できます。

OS エラーの原因をプラットフォーム共通で調べる場合は、`error.h` の `COM_UTIL_CAUSE_*` を使用します。  
要因コードは `com_util_error_get_cause()` または `com_util_error_is()` による詳細エラーの解釈専用であり、関数の戻り値には使用しません。  
新しい要因を追加する場合は、errno と Win32 エラー コードの両方の対応表を確認します。

`result.h` は、粗い分類 (`COM_UTIL_ERR_INVALID_ARGUMENT` など) と細かい操作結果 (`COM_UTIL_ERR_UNKNOWN_OPTION` など) の両方を含みます。  
モジュール固有の戻り値コード体系は別に設けません。  
argparser の `_com_util_argparser_parse()` は解析エラーの種別に対応するコードを直接返し、`_com_util_argparser_get_error()` はその種別を後から再取得する用途で提供しています。

### OS エラー詳細の抽象化

OS 由来の詳細は、`com_util_error` にドメイン、対応する共通結果コード、生のエラー値をまとめて保持します。  
公開 API の引数や戻り値で、生の `errno` または `GetLastError()` の値だけを受け渡してはなりません。  
自前の OS 呼び出しで得た値は `com_util_error_capture_errno()` または `com_util_error_capture_windows_error()` で取り込みます。

`detail_out` を持つ API は、失敗時に出力引数と現在のスレッドの直前値へ同じ詳細を記録し、成功時に両方をクリアします。  
`detail_out` へ `NULL` を指定した場合、本引数へはエラー詳細を設定せず、返却しませんが、スレッドの直前値は更新されます。  
`com_util_error_get_last()` の値は、次に対応 API を呼び出すと更新されるため、保持が必要な場合は直ちにコピーするか `detail_out` を使用します。
OS API の失敗後に後処理を行うアダプターは、先に詳細を保存し、後処理で直前値が変化した場合に `com_util_error_set_last()` で保存値を復元します。
`com_util_error_set_last()` には有効な保存値だけを指定し、NULL を指定した場合は現在のスレッドの直前値をクリアします。

失敗の原因が OS 呼び出しに由来しない場合 (引数の検証エラー、パターンの構文エラーなど) は、`com_util_error` に格納すべき詳細が存在しません。  
この場合は `detail_out` と直前値の双方をクリアし、原因は共通結果コードだけで表します。  
`com_util_error_is_set()` が偽であることは「OS 由来の詳細がない」ことを意味し、成功したことを意味しません。呼び出し側は必ず戻り値で成否を判定します。

`com_util_error_get_cause()` は OS ごとの差を吸収した原因判定に使用し、`com_util_error_to_result()` は詳細を共通結果コードへ変換する場合に使用します。  
人間可読の文字列は `com_util_error_message()` で取得し、公開 API から生の OS エラー値を直接文字列化しません。

### スレッド ローカル記憶域

com_util 内部のスレッド ローカル変数には `compiler.h` の `THREAD_LOCAL` を使用します。  
TLS 変数はソース ファイル内のファイル スコープ `static` に限定し、ヘッダーで `extern` 宣言しません。  
同一プロセスで com_util の静的ライブラリ版と動的ライブラリ版を混在させると直前値が複数に分かれるため、混在させてはなりません。  
Linux の共有ライブラリおよび `dlopen()` による読み込みへ対応するため、`-ftls-model=initial-exec` や `-ftls-model=local-exec` を指定しません。

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

## ラッパーの設計方針

### ラッパーを作る条件

CRT / POSIX / Win32 関数のラッパーを com_util へ追加してよいのは、次のいずれかを満たす場合に限ります。

| 条件 | 例 |
|---|---|
| プラットフォームで異なる API を呼び分ける | `com_util_fseek` (`fseeko` と `_fseeki64`)、`com_util_gmtime` (`gmtime_r` と `gmtime_s`) |
| Windows で UTF-8 と UTF-16 を変換する | `com_util_fopen`、`com_util_access`、`CreateFileU` |
| 戻り値やエラー伝達の規約を正規化する | `com_util_dup2` (POSIX は newfd、Windows は 0 を返すため 0 へ統一)、`com_util_strcpy` (`ERANGE` を返す) |
| MSVC のセキュア版と意味のある挙動差がある | `com_util_vfprintf` (`vfprintf_s` は不正な書式を拒否する) |
| com_util 定義の型を扱う | `com_util_timespec_*`、`com_util_file_*` |
| com_util の他機能と統合する必要がある | `com_util_exit` (登録済みシャットダウン コールバックを実行する) |

### ラッパーを作らないもの

上記のいずれにも当たらず、両プラットフォームで同じ関数を素通しするだけのラッパーは作りません。  
すでに存在する場合は撤去します。

`memcpy`、`memmove`、`memset`、`strcmp` 系、`malloc` 系、標準出力への `printf` が該当します。  
**利用件数の多さは抽象価値の根拠になりません。** これらはリポジトリ内で最も多く使われていますが、プラットフォーム差異がなく、MSVC のセキュア版にも意味のある挙動差がないため対象外です。

テストでモックしたいという理由も、com_util 側にラッパーを作る根拠にはなりません。  
libc 関数のモックは `framework/testfw/libsrc/mock_libc/` が提供する仕組みで受けます。

### ラッパーがある関数の使用

ラッパーを提供している関数は、`app/` 配下のすべてのコードでラッパーを使用します。  
com_util 自身の実装 (`prod/libsrc/`) も対象に含みます。

例外は、そのラッパー自身の実装だけです。  
ラッパーが元関数へ委譲する箇所は元関数を直接呼び出します。

`mock_com_util` は `com_util_*` を weak シンボルで差し替えるため、実装内部の呼び出しもモックの対象になります。  
モック未設定時は実関数へ委譲されます。

### scanf 系ラッパー

`scanf`、`fscanf`、`sscanf` と各 `v*` 版は、com_util の対応するラッパーを使用します。  
これらの API は Linux の scanf 書式と可変長引数の契約を正とし、Windows でも `_s` 版へ切り替えません。

`%s`、`%S`、`%[` で文字列を格納するときは、終端文字を除いた最大文字数を幅として指定します。  
幅は宛先バッファーの要素数より小さくなければなりません。  
`%c`、`%C` は終端文字を追加しないため、指定幅以上の要素数を持つ宛先を渡します。

非信頼なストリーム入力は `fgets` で 1 行を読み取ってから `com_util_sscanf` で解析します。  
`com_util_scanf` と `com_util_fscanf` は、既存の scanf 形式との互換が必要な場合に使用します。

Coverity では、幅なし `%s`、宛先容量以上の幅、com_util ラッパー経由の呼び出しをそれぞれ検出できることを、利用する製品版で確認します。  
ASan では、幅指定を欠くテスト用入力が境界外書き込みとして報告されることを手動で確認します。  
どちらも書式が定数で、解析対象に呼び出し元とラッパー実装が含まれる場合に検出を期待できます。

### Win32 API の UTF-8 ラッパーの適用範囲

`CreateFileU` などの `*U` ラッパーは、UTF-8 文字列と Win32 の UTF-16 API の境界を吸収するためのものです。  
したがって、`*U` を使うのは **その呼び出し箇所に UTF-8 との境界が実際に存在する場合** に限ります。

`*W` を直接呼び出してよいのは次の場合です。

- 引数が `L"CONOUT$"` のようなワイド文字列リテラルであり、UTF-8 の文字列が関与しない
- 呼び出し前後の処理がワイド文字列のまま完結しており、`*U` を経由すると UTF-8 への往復変換が新たに発生する
- 構造体引数に文字列メンバーが含まれない (`SERVICE_PRESHUTDOWN_INFO` など)

いずれの場合も、`*U` を使わない理由を該当箇所へコメントとして残します。  
コメントがないと、将来の点検で「`*U` への置換漏れ」と誤認されます。

逆に、`*A` 版はコンソールのコード ページ (日本語環境では cp932) で文字列を解釈するため、使用しません。  
`*A` を使っている箇所は非 ASCII 文字で壊れるため、`*U` へ置き換えます。

### 新設 API 群の規約

- セキュア消去は volatile 経由で書き込み、コンパイラによる最適化除去を防ぎます。非 volatile のループや素の `memset` は使用しません
- 乱数は暗号論的乱数源のみを使用し、取得に失敗した場合は結果コードで通知します。呼び出し側が失敗を無視して処理を続行しない設計とします
- OS エラーの文字列化は `com_util_error_message()` がドメインに基づいて処理を振り分けます。生の errno と Win32 エラー コードを同一の整数引数で受け取る公開 API は作りません

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
`com_util_error` のような値型構造体には `_t` を付けず、その分類に使用する enum 型には `com_util_error_domain_t` のように `_t` を付けます。

## 引数順序規約

com_util の公開 API の引数順序は、API の性格に応じて以下の 3 規則に従います。

### 変換・整形系 (COM_UTIL_OK 系)

入力を出力バッファーへ変換・整形する API は、CRT の `strcpy_s` 系に合わせて出力バッファーを先頭に置きます。

```c
戻り値 関数名(out[, out_size][, detail_out], 入力...);
```

```c
com_util_strcpy(dest, dest_size, src);
com_util_path_dirname(path_out, path_size, detail_out, path);
com_util_gmtime(utc_tm, timep);
com_util_stat(buf, detail_out, path);
```

`com_util_stat` が POSIX の `stat(path, buf)` と逆順であるのは、本規約 (出力先頭) によるものです。  
`detail_out` を提供する場合は、出力引数と、その出力サイズがある場合は出力サイズの直後に置き、後続の入力引数より前に置きます。

### ハンドル・操作系 (COM_UTIL_OK 系)

ハンドルまたは操作対象を先頭に置き、`*_out` の出力引数は末尾に置きます。

```c
com_util_file_get_size(file, size_out, detail_out);
com_util_paths_equal(lhs, rhs, equal_out, detail_out);
com_util_elevated_process_run_with_result(arguments, exit_code, handled, result_message, result_message_size);
```

### 適用対象外 API と _fmt 系

「エラー処理と戻り値規約」の適用対象外 API は、元 API の引数順を保存し、追加の出力引数 (`detail_out` など) は末尾に付加します。

```c
com_util_fopen(path, modes, detail_out);        /* fopen(path, modes) + detail_out */
com_util_fopen_temp(prefix, modes, path_out, path_size, detail_out);
```

`_fmt` 系はパス引数を書式で組み立てる派生 API であり、基底 API からパス引数を除いた残りの引数順を維持し、末尾に `format` と可変長引数を置きます。  
`v*_fmt` は可変長引数を `va_list args` に置き換えます。

```c
com_util_open(path, flags, mode, detail_out);
com_util_open_fmt(flags, mode, detail_out, format, ...);
com_util_vopen_fmt(flags, mode, detail_out, format, args);
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
