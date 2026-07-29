# API 規約適合の移行ガイド (2026-07 実施分)

## 概要

com_util は、[`coding-guideline.md`](coding-guideline.md) の「API 命名規約」「引数順序規約」および上位「コーディング規範」への適合のため、「既知の逸脱と移行課題」に整理していた公開 API を一括変更しました。  
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

### 戻り値型のみ変更 (ソース互換、再ビルドのみ必要)

argparser の既定パーサー ラッパー 15 関数は、`void` 戻りから明示ハンドル版と同じ `int` (結果コード) 戻りへ変更しました。

- `com_util_argparser_register_flag` / `register_option_int` / `register_option_string` / `register_option_int_array` / `register_option_string_array` / `register_positional_int` / `register_positional_string` / `register_positional_int_array` / `register_positional_string_array`
- `com_util_argparser_get_error_message` / `get_usage` / `print_usage` / `print_error_messages` / `get_register_error_message` / `print_register_error_messages`

戻り値を無視する既存の呼び出しはそのままコンパイル・動作しますが、ABI (戻り値レジスターの意味) が変わるため、利用側の再ビルドが必要です。  
登録エラーの一括確認 (`com_util_argparser_get_register_error_count`) は引き続き使用できます。

`com_util_argparser_init` は変更していません。  
既定パーサーはライブラリが所有し、初期化後はプロセス終了まで常に有効で、利用側による破棄を必要としない設計です (`coding-guideline.md` の「生成と破棄の動詞対」)。

## 移行手順

1. **ビルドで機械的に検出**: `com_util_getenv`、`com_util_pinned_prompt_write`、`com_util_etw_session_start` の呼び出しと `com_util_process_options_t` / `com_util_process_stdio_t` の参照は、引数個数・型名の変更によりコンパイル エラーとして検出されます。
2. **getenv の判定式の書き換え**: 旧コードの「`戻り値 != 0` なら未設定またはエラー」の判定は、新コードでは成立しません。設定有無の判定は `exists_out` で行います。

   ```c
   /* 旧 */
   if (com_util_getenv(name, buf, sizeof(buf)) != 0) { /* 未設定または不足 */ }

   /* 新 */
   int exists = 0;
   if (com_util_getenv(name, buf, sizeof(buf), &exists) != 0 || exists == 0) { /* 未設定または不足 */ }
   ```

3. **mock の追随**: `mock_com_util` を利用するテストで、上記 API の `EXPECT_CALL` / `ON_CALL` の引数個数と戻り値型を新シグネチャへ更新します。
4. **ローカル テストで確認**: `make -C app/<repo> test` で回帰がないことを確認します。

## 参考: 本リポジトリ内で追随済みの利用側

- `app/bench-io/prod/src/cmd/bench-io/bench_report.c` (`com_util_getenv`)
- `app/service-sample/prod/src/cmd/service-sample/service-sample_linux.c` (`com_util_process_options`)
- `app/com_util/prod/include/com_util/base/shared_lib_lifecycle.h` (`com_util_getenv`)
- `app/com_util/prod/src/cmd/etw-viewer/etw-viewer.c` (`com_util_etw_session_start`)
