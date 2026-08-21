# API 規約適合の移行ガイド (2026-07 実施分)

> [!NOTE]
> 本書は、2026 年 7 月に完了した移行の履歴と、旧 API を利用するコード向けの対応表です。
> 現行規則は [com_util コーディング規範](coding-guideline.md)、現行 API は [API チート シート](api-cheatsheet.md) を参照してください。

## 概要

com_util は、[`coding-guideline.md`](coding-guideline.md) の「API 命名規約」「引数順序規約」および上位「コーディング規範」への適合のため、「既知の逸脱と移行課題」に整理していた公開 API を一括変更しました。  
あわせて argparser の詳細コードを共通結果コードへ統合し、結果コードの値を再採番しました。  
本書は、com_util を利用するリポジトリが本変更へ追随する際の対応表と移行手順を示します。  
戻り値規約そのものの移行は [`result-code-migration.md`](result-code-migration.md) を参照してください。

## 旧 → 新の対応表

### シグネチャ変更 (コンパイル エラーで検出可能)

| API | 旧シグネチャ / 意味 | 新シグネチャ / 意味 |
|---|---|---|
| `com_util_getenv` | `(name, buf, buf_size)`。戻り値 0=設定あり / -1=未設定 / `ERANGE`=不足 | `(name, buf, buf_size, int *exists_out)`。戻り値 0=成功 / `EINVAL`=引数不正 / `ERANGE`=不足。設定有無は `exists_out` (NULL 可)。未設定時は `buf` に空文字列を格納 |
| `com_util_pinned_prompt_write` | `(screen, channel, data, size)`。戻り値 `size_t` (書き込みバイト数、引数不正は 0) | `(screen, channel, data, size, size_t *written_out)`。戻り値は結果コード (`COM_UTIL_OK` / `COM_UTIL_ERR_INVALID_ARGUMENT` / 短縮書き込み時 `COM_UTIL_ERR_UNKNOWN`)。バイト数は `written_out` (NULL 可) |
| `com_util_etw_session_start` | `(session_name, guid, callback, context, int *out_status)`。戻り値はハンドル (失敗時 NULL + `out_status`) | `(session_name, guid, callback, context, com_util_etw_session **session_out)`。戻り値は結果コード、ハンドルは `session_out` (成功時のみ有効) |
| `com_util_process_options_t` / `com_util_process_stdio_t` | typedef struct への `_t` 別名 | `_t` なしのタグ名 `com_util_process_options` / `com_util_process_stdio` へ統一 (別名は廃止) |

### 型名変更 (enum / 関数ポインター、コンパイル エラーで検出可能)

POSIX が予約する `_t` サフィックスを、公開 enum と関数ポインター typedef から除去しました。  
関数ポインターは `_func` / `_callback` を使わず、サフィックスを `_fn` に統一しています。  
互換エイリアスは置かないため、旧型名の参照はコンパイル エラーになります。

#### enum (13 型)

| 旧名 | 新名 |
|---|---|
| `com_util_shutdown_reason_t` | `com_util_shutdown_reason` |
| `com_util_shutdown_code_kind_t` | `com_util_shutdown_code_kind` |
| `com_util_trace_level_t` | `com_util_trace_level` |
| `com_util_tracer_state_t` | `com_util_tracer_state` |
| `com_util_error_domain_t` | `com_util_error_domain` |
| `com_util_error_cause_t` | `com_util_error_cause` |
| `com_util_stream_t` | `com_util_stream` |
| `com_util_mmap_access_t` | `com_util_mmap_access` |
| `com_util_pinned_prompt_channel_t` | `com_util_pinned_prompt_channel` |
| `com_util_pinned_prompt_status_position_t` | `com_util_pinned_prompt_status_position` |
| `com_util_pinned_prompt_status_align_t` | `com_util_pinned_prompt_status_align` |
| `com_util_process_stdio_mode_t` | `com_util_process_stdio_mode` |
| `com_util_interprocess_sync_backend_t` | `com_util_interprocess_sync_backend` |

#### 関数ポインター (5 型)

| 旧名 | 新名 |
|---|---|
| `com_util_thread_func_t` | `com_util_thread_fn` |
| `com_util_once_func_t` | `com_util_once_fn` |
| `com_util_shutdown_callback_t` | `com_util_shutdown_fn` |
| `com_util_etw_event_callback_t` | `com_util_etw_event_fn` |
| `com_util_tracer_hook_fn_t` | `com_util_tracer_hook_fn` |

#### _t を維持する例外 (2 型)

OS / SDK が定義する型の alias に限り、`_t` を維持します。  
理由の詳細は [`coding-guideline.md`](coding-guideline.md) の「typedef の規則」を参照してください。

| 型 | 由来 |
|---|---|
| `com_util_file_stat_t` | POSIX `struct stat` / MSVC `struct _stat64` の alias |
| `com_util_etw_provider_ref_t` | Windows TraceLogging SDK 内部型への参照の alias |

### 戻り値型のみ変更 (ソース互換、再ビルドのみ必要)

argparser の既定パーサー ラッパー 15 関数は、`void` 戻りから明示ハンドル版と同じ `int` (結果コード) 戻りへ変更しました。

- `com_util_argparser_default_register_flag` / `register_option_int` / `register_option_string` / `register_option_int_array` / `register_option_string_array` / `register_positional_int` / `register_positional_string` / `register_positional_int_array` / `register_positional_string_array`
- `com_util_argparser_default_get_error_message` / `get_usage` / `print_usage` / `print_error_messages` / `get_register_error_message` / `print_register_error_messages`

戻り値を無視する既存の呼び出しはそのままコンパイル・動作しますが、ABI (戻り値レジスターの意味) が変わるため、利用側の再ビルドが必要です。  
登録エラーの一括確認 (`com_util_argparser_default_get_register_error_count`) は引き続き使用できます。

`com_util_argparser_default_init` は変更していません。  
既定パーサーはライブラリが所有し、初期化後はプロセス終了まで常に有効で、利用側による破棄を必要としない設計です (`coding-guideline.md` の「生成と破棄の動詞対」)。

## 詳細コードの共通結果コードへの統合

argparser の詳細コード `COM_UTIL_ARGPARSER_ERROR_*` を廃止し、共通結果コード (`result.h`) へ統合しました。  
あわせて定義を課題別の帯へ再編したため、**すべての結果コードの値が変わりました**。値は再凍結しています。

### 廃止したコードと対応先

| 旧 (0 以上の詳細コード) | 新 (負値の共通結果コード) |
|---|---|
| `COM_UTIL_ARGPARSER_ERROR_NONE` (0) | `COM_UTIL_OK` (0) |
| `COM_UTIL_ARGPARSER_ERROR_UNKNOWN_OPTION` (1) | `COM_UTIL_ERR_UNKNOWN_OPTION` (-20) |
| `COM_UTIL_ARGPARSER_ERROR_MISSING_VALUE` (2) | `COM_UTIL_ERR_MISSING_VALUE` (-21) |
| `COM_UTIL_ARGPARSER_ERROR_UNEXPECTED_VALUE` (9) | `COM_UTIL_ERR_UNEXPECTED_VALUE` (-22) |
| `COM_UTIL_ARGPARSER_ERROR_INVALID_INT` (3) | `COM_UTIL_ERR_INVALID_INTEGER` (-23) |
| `COM_UTIL_ARGPARSER_ERROR_OUT_OF_RANGE` (4) | `COM_UTIL_ERR_OUT_OF_RANGE` (-24) |
| `COM_UTIL_ARGPARSER_ERROR_MISSING_REQUIRED` (5) | `COM_UTIL_ERR_MISSING_REQUIRED` (-25) |
| `COM_UTIL_ARGPARSER_ERROR_DUPLICATE_OPTION` (6) | `COM_UTIL_ERR_DUPLICATE_OPTION` (-26) |
| `COM_UTIL_ARGPARSER_ERROR_TOO_MANY_POSITIONALS` (7) | `COM_UTIL_ERR_TOO_MANY_ARGUMENTS` (-27) |
| `COM_UTIL_ARGPARSER_ERROR_TOO_MANY_OCCURRENCES` (8) | `COM_UTIL_ERR_TOO_MANY_OCCURRENCES` (-28) |

`COM_UTIL_ERR_PARSE` は削除しました。解析エラーは上表の具体コードで表します。

### 値が変わった既存コード

| コード | 旧値 | 新値 |
|---|---|---|
| `COM_UTIL_ERR_OUT_OF_MEMORY` | -4 | -10 |
| `COM_UTIL_ERR_PERMISSION_DENIED` | -5 | -4 |
| `COM_UTIL_ERR_TIMEOUT` | -6 | -12 |
| `COM_UTIL_ERR_BUSY` | -7 | -11 |
| `COM_UTIL_ERR_BUFFER_TOO_SMALL` | -8 | -14 |
| `COM_UTIL_ERR_LIMIT_EXCEEDED` | -9 | -13 |
| `COM_UTIL_ERR_CORRUPT_DESCRIPTOR` | -10 | -15 |
| `COM_UTIL_ERR_DUPLICATE_DEFINITION` | -11 | -5 |
| `COM_UTIL_ERR_EOF` | -13 | -40 |
| `COM_UTIL_ERR_CANCELED` | -14 | -41 |

`COM_UTIL_OK` (0)、`COM_UTIL_ERR_UNKNOWN` (-1)、`COM_UTIL_ERR_INVALID_ARGUMENT` (-2)、`COM_UTIL_ERR_UNSUPPORTED` (-3) は変わりません。

### parse の戻り値

`com_util_argparser_parse()` と `com_util_argparser_default_parse()` は、従来つねに `COM_UTIL_ERR_PARSE` を返し、種別は `get_error()` で取得する二段構えでした。  
統合により、解析エラーの種別に対応するコードを直接返すようになりました。  
`com_util_argparser_get_error()` は種別を後から再取得する用途で引き続き利用できます。戻り値の符号が 0 以上から負値に変わっています。

## 移行手順

1. **ビルドで機械的に検出**: `com_util_getenv`、`com_util_pinned_prompt_write`、`com_util_etw_session_start` の呼び出しと、`com_util_process_options_t` / `com_util_process_stdio_t` および本節の enum / 関数ポインター旧型名の参照は、引数個数・型名の変更によりコンパイル エラーとして検出されます。
2. **getenv の判定式の書き換え**: 旧コードの「`戻り値 != 0` なら未設定またはエラー」の判定は、新コードでは成立しません。設定有無の判定は `exists_out` で行います。

   ```c
   /* 旧 */
   if (com_util_getenv(name, buf, sizeof(buf)) != 0) { /* 未設定または不足 */ }

   /* 新 */
   int exists = 0;
   if (com_util_getenv(name, buf, sizeof(buf), &exists) != 0 || exists == 0) { /* 未設定または不足 */ }
   ```

3. **旧詳細コードの置換**: `COM_UTIL_ARGPARSER_ERROR_*` と `COM_UTIL_ERR_PARSE` はマクロ自体を削除したため、参照はコンパイル エラーとして検出されます。上表に従って置換します。
4. **結果コードの数値リテラル比較の洗い出し**: 値が変わったため、コード名ではなく数値で比較している箇所は意味が変わります。以下で洗い出し、コード名との比較へ書き換えます。

   ```bash
   grep -nE '(==|!=|<=|>=)[[:space:]]*\(?-[0-9]+\)?' <対象ディレクトリ>/*.c
   ```

5. **利用側の再ビルド**: 値が変わったため、com_util を利用するすべてのモジュールを再ビルドします。ヘッダーだけを更新して再リンクする運用では不整合が生じます。
6. **mock の追随**: `mock_com_util` を利用するテストで、シグネチャが変わった API の `EXPECT_CALL` / `ON_CALL` の引数個数と戻り値型を新シグネチャへ更新します。
7. **ローカル テストで確認**: `make -C app/<repo> test` で回帰がないことを確認します。

## API 名変更 (2026-08 実施分: 生成・破棄動詞の統一)

[`coding-guideline.md`](coding-guideline.md) の「生成と破棄の動詞対」節に従い、「ハンドルを完全に破棄し二度と使えなくする」という意味を持つ破棄動詞 `*_destroy` を `*_dispose` へ統一しました。  
シグネチャは変わらないため、コンパイル エラーではなくリンク エラー (未定義シンボル) として検出されます。  
`*_detach`・`*_close`・`*_stop`・`*_release` は意味が異なるため対象外です (詳細は `coding-guideline.md` を参照)。

| 旧名 | 新名 |
|---|---|
| `com_util_hashtable_destroy` | `com_util_hashtable_dispose` |
| `com_util_process_destroy` | `com_util_process_dispose` |
| `com_util_local_lock_destroy` | `com_util_local_lock_dispose` |
| `com_util_condvar_destroy` | `com_util_condvar_dispose` |
| `com_util_local_rwlock_destroy` | `com_util_local_rwlock_dispose` |
| `com_util_interprocess_lock_destroy` | `com_util_interprocess_lock_dispose` |
| `com_util_interprocess_rwlock_destroy` | `com_util_interprocess_rwlock_dispose` |

## 関連ガイド

OS 由来の詳細値を `int *errno_out` からドメイン付きの `com_util_error` へ移行する手順は、[`error-detail-migration.md`](error-detail-migration.md) を参照してください。
