# com_util_error 移行ガイド

> [!NOTE]
> 本書は、`int *errno_out` から `com_util_error *detail_out` への完了済み移行を説明する履歴文書です。
> 現行規則は [com_util コーディング規範](coding-guideline.md)、現行 API は [API チート シート](api-cheatsheet.md) を参照してください。

## 概要

com_util の OS エラー詳細は、生の `errno` を格納する `int *errno_out` から、ドメイン付きの値型 `com_util_error *detail_out` へクリーン ブレークで移行しました。  
互換用の旧シグネチャや型エイリアスは提供しません。

本変更は、戻り値を `COM_UTIL_OK` / `COM_UTIL_ERR_*` へ統一した [`result-code-migration.md`](result-code-migration.md) と、公開 API の一貫性を改善した [`api-consistency-migration.md`](api-consistency-migration.md) に続く移行です。  
戻り値は操作結果、`com_util_error` は OS 由来の詳細、`COM_UTIL_CAUSE_*` は詳細をプラットフォーム共通で判定する語彙として役割を分けます。

## 新しいエラー詳細の扱い

`com_util_error` は、次の情報を 1 つの値として保持します。

| メンバー | 内容 |
|---|---|
| `domain` | `COM_UTIL_ERROR_DOMAIN_NONE`、`COM_UTIL_ERROR_DOMAIN_ERRNO`、`COM_UTIL_ERROR_DOMAIN_WINDOWS`、`COM_UTIL_ERROR_DOMAIN_SOCKET_ERRNO`、`COM_UTIL_ERROR_DOMAIN_WINSOCK`、`COM_UTIL_ERROR_DOMAIN_GAI` のいずれか |
| `result` | 生の OS エラー値に対応する `COM_UTIL_ERR_*` |
| `code` | ドメイン固有の生のエラー値 |

`detail_out` を持つ com_util API は、失敗時に出力引数と現在のスレッドの直前値へ同じ詳細を記録します。  
成功時は出力引数と直前値をクリアします。  
`detail_out` に `NULL` を指定した場合、本引数へはエラー詳細を設定せず、返却しませんが、直前値は更新されます。  
既存コードで詳細が不要な場合は、従来どおり `NULL` を指定できます。

直前値は `com_util_error_get_last()`、要因は `com_util_error_get_cause()` または `com_util_error_is()`、人間可読の文字列は `com_util_error_message()` で取得します。

## シグネチャを変更した API

次の表では、変更点に関係する引数だけを示します。

| API | 旧シグネチャの詳細引数 | 新シグネチャの詳細引数 |
|---|---|---|
| `com_util_path_get_full` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_paths_equal` | `int *equal_out, int *errno_out` | `int *equal_out, com_util_error *detail_out` |
| `com_util_get_temp_dir` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_path_concat_n` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_path_dirname` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_path_strip_extension` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_path_join_n` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_fopen` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_freopen` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_fopen_fmt` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_vfopen_fmt` | `int *errno_out` | `com_util_error *detail_out` |
| `com_util_fopen_temp` | `int *errno_out` | `com_util_error *detail_out` |

今回の追加調査では、次の API に `com_util_error *detail_out` を追加しました。

- `com_util_remove`、`com_util_rename`、`com_util_remove_fmt`、`com_util_vremove_fmt`
- `com_util_open`、`com_util_open_fmt`、`com_util_vopen_fmt`
- `com_util_lseek`、`com_util_close`、`com_util_dup`、`com_util_dup2`、`com_util_read`、`com_util_write`、`com_util_access`、`com_util_access_fmt`、`com_util_vaccess_fmt`
- `com_util_stat`、`com_util_mkdir`、`com_util_makedirs`、`com_util_rmdir`、`com_util_stat_fmt`、`com_util_vstat_fmt`、`com_util_mkdir_fmt`、`com_util_vmkdir_fmt`
- `com_util_getenv`、`com_util_setenv`、`com_util_unsetenv`
- `com_util_file_open`、`com_util_file_write`、`com_util_file_read`、`com_util_file_get_size`、`com_util_file_set_size`、`com_util_file_get_id`、`com_util_file_get_path_id`、`com_util_file_close`
- `com_util_mmap_attach`、`com_util_mmap_get_rwlock`、`com_util_mmap_flush`、`com_util_mmap_detach`

`com_util_file_close` と `com_util_mmap_detach` は、解放処理の失敗を通知するため、戻り値を `void` から共通結果コードへ変更しました。  
`com_util_mmap_get_rwlock` は、ロック ポインターを戻り値から `lock_out` へ移し、戻り値で共通結果コードを返します。  
`com_util_stat` は変換・整形系の引数順序規約に従い、`com_util_stat(buf, detail_out, path)` の順序で呼び出します。

新たに `com_util_fclose`、`com_util_fflush`、`com_util_fread`、`com_util_fwrite`、`com_util_file_flush` を追加しました。  
標準 I/O ラッパーは元の CRT と同じ戻り値規約を保ち、OS エラー詳細だけを抽象化します。

`com_util_getenv`、`com_util_setenv`、`com_util_unsetenv` と文字列コピー API は、生の errno 値ではなく共通結果コードを返します。  
文字列コピー API は OS エラーを発生させないため、`detail_out` を追加していません。

当初の調査では 9 API を対象としていましたが、同じヘッダーにある `com_util_freopen`、`com_util_fopen_temp`、`com_util_paths_equal` を含めると 12 API でした。  
同一ヘッダーに異なる詳細エラー規約を残さないため、12 API を同時に変更しました。

`com_util_path_concat()` と `com_util_path_join()` は、それぞれ `com_util_path_concat_n()` と `com_util_path_join_n()` を呼ぶマクロです。  
これらのマクロへ渡す詳細引数も `com_util_error *` に変わります。

## コンパイルでは検出できない挙動変更

旧 API は成功時に `errno_out` の値を変更しない場合がありました。  
新 API は成功時に `detail_out` とスレッド ローカルの直前値を必ずクリアします。

成功後も以前の失敗値が残ることを前提にしたコードは、コンパイルが成功しても挙動が変わります。  
失敗の詳細を後続 API の呼び出し後も保持する場合は、対象 API の `detail_out` を使用するか、失敗直後に `com_util_error_get_last()` で値をコピーしてください。

## 書き換え例

### 詳細が不要な場合

`NULL` の指定方法は変わりません。

```c
int rc = com_util_path_get_full(path_out, sizeof(path_out), NULL, path);
```

### 生の errno を受け取っていた場合

```c
/* 旧 */
int errno_value = 0;
FILE *file = com_util_fopen(path, "rb", &errno_value);

/* 新 */
com_util_error detail;
FILE *file = com_util_fopen(path, "rb", &detail);
```

### OS に依存しない原因判定

```c
if (file == NULL && com_util_error_is(&detail, COM_UTIL_CAUSE_NOT_FOUND) != 0)
{
    /* Linux の ENOENT と Windows の ERROR_FILE_NOT_FOUND を同じ意味で扱う。 */
}
```

生の `ENOENT` との比較は、`COM_UTIL_CAUSE_NOT_FOUND` の判定へ置き換えます。  
同様に、OS ごとのエラー定数を並べた分岐は、対応する `COM_UTIL_CAUSE_*` がある場合に要因判定へ置き換えます。

### エラー メッセージの取得

```c
char message[256];
com_util_error detail;
FILE *file = com_util_fopen(path, "rb", &detail);
if (file == NULL)
{
    (void)com_util_error_message(message, sizeof(message), &detail);
}
```

旧 `com_util_errno_message()` または `com_util_win32_error_message()` の呼び出しは、詳細値の取り込みと `com_util_error_message()` の組み合わせへ変更します。

### 自前の OS 呼び出し結果を取り込む場合

```c
com_util_error detail;

if (os_operation_failed)
{
    com_util_error_capture_current_errno(&detail);
}
```

明示的に保存した値を取り込む場合は、`com_util_error_capture_errno()` を使用します。  
Windows では、失敗した OS API の直後に `com_util_error_capture_current_windows_error()` を呼び出します。  
明示的に保存した `GetLastError()` の値を取り込む場合は、`com_util_error_capture_windows_error()` を使用します。  
メッセージ生成などの別 API を先に呼ぶと、OS の直前値が上書きされる可能性があります。

生の値が必要な場合は、ドメインを確認した後で `com_util_error_get_errno()` または `com_util_error_get_windows_error()` を使用します。

## 公開エラー文字列 API の変更

`com_util_errno_message()` と `com_util_win32_error_message()` は公開 API から削除しました。  
新しい公開 API は `com_util_error_message()` だけであり、`com_util_error` のドメインに基づいて処理を振り分けます。

生の数値だけを受け取る API は、呼び出し側が errno と Win32 エラー コードを取り違えても型や値から検出できません。  
ドメイン付きの値を境界にすることで、誤ったエラー体系による文字列化を防ぎます。

## TLS の利用と制限

直前値はスレッドごとに独立しており、別スレッドの失敗や成功では変更されません。  
ただし、同じスレッドで次の対応 API を呼び出すと、成功か失敗かにかかわらず更新されます。

確実に保持する必要がある詳細には `detail_out` を使用してください。  
`com_util_error_get_last()` は、既存の API 呼び出し形を保ったまま補助的に詳細を取得する用途に適しています。  
失敗後の後処理によって直前値が変化するアダプターは、失敗の詳細を先にコピーし、後処理後に `com_util_error_set_last()` で復元できます。  
NULL を指定すると、現在のスレッドの直前値をクリアします。

同一プロセスで com_util の静的ライブラリ版と動的ライブラリ版を混在させると、それぞれが別の TLS を持ちます。  
同じ実行ファイル内で両方を使用してはなりません。

## 修正された Windows 固有の問題

旧 `com_util_fopen_temp()` は一部の Windows エラーで `GetLastError()` の値を `int *errno_out` へ格納していました。  
呼び出し側がその値を errno として文字列化すると、無関係なメッセージになる可能性がありました。

新 API は Windows の失敗を `COM_UTIL_ERROR_DOMAIN_WINDOWS` として保持します。  
`com_util_error_message()` はドメインを確認して Win32 エラーとして文字列化します。

## 移行手順

1. 利用コードをビルドし、12 API の引数型変更によるコンパイル エラーを確認します。
2. `int errno_value` などの受け取り先を `com_util_error detail` へ変更します。
3. `ENOENT` などとの比較を、対応する `COM_UTIL_CAUSE_*` の判定へ変更します。
4. 生のエラー文字列 API を `com_util_error_message()` へ変更します。
5. 成功後に以前のエラー値が残ることを前提にしていないか確認します。
6. mock の `EXPECT_CALL` と `ON_CALL` の引数型を更新します。
7. 利用モジュールの局所ビルドと局所テストを実行します。

残存参照は、次の検索で確認できます。

```bash
rg -n 'errno_out|com_util_errno_message|com_util_win32_error_message' <対象ディレクトリ>
rg -n 'com_util_(path_get_full|paths_equal|get_temp_dir|path_concat_n|path_dirname|path_strip_extension|path_join_n|fopen|freopen|fopen_fmt|vfopen_fmt|fopen_temp)' <対象ディレクトリ>
```

## 今回の対象外

`sync` と `runtime` の全 API へ `detail_out` を追加する変更は、今回の対象外です。  
これらへ詳細エラー抽象化を展開する場合も、戻り値、`detail_out`、TLS の役割分担と成功時クリアの規約を維持します。
