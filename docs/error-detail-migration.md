# cplat_error 移行ガイド

> [!NOTE]
> 本書は、`int *errno_out` から `cplat_error *detail_out` への完了済み移行を説明する履歴文書です。
> 現行規則は [cplat コーディング規範](coding-guideline.md)、現行 API は [API チート シート](api-cheatsheet.md) を参照してください。

## 概要

cplat の OS エラー詳細は、生の `errno` を格納する `int *errno_out` から、ドメイン付きの値型 `cplat_error *detail_out` へクリーン ブレークで移行しました。  
互換用の旧シグネチャや型エイリアスは提供しません。

本変更は、戻り値を `CPLAT_OK` / `CPLAT_ERR_*` へ統一した [`result-code-migration.md`](result-code-migration.md) と、公開 API の一貫性を改善した [`api-consistency-migration.md`](api-consistency-migration.md) に続く移行です。  
戻り値は操作結果、`cplat_error` は OS 由来の詳細、`CPLAT_CAUSE_*` は詳細をプラットフォーム共通で判定する語彙として役割を分けます。

## 新しいエラー詳細の扱い

`cplat_error` は、次の情報を 1 つの値として保持します。

| メンバー | 内容 |
|---|---|
| `domain` | `CPLAT_ERROR_DOMAIN_NONE`、`CPLAT_ERROR_DOMAIN_ERRNO`、`CPLAT_ERROR_DOMAIN_WINDOWS`、`CPLAT_ERROR_DOMAIN_SOCKET_ERRNO`、`CPLAT_ERROR_DOMAIN_WINSOCK`、`CPLAT_ERROR_DOMAIN_GAI` のいずれか |
| `result` | 生の OS エラー値に対応する `CPLAT_ERR_*` |
| `code` | ドメイン固有の生のエラー値 |

`detail_out` を持つ cplat API は、失敗時に出力引数と現在のスレッドの直前値へ同じ詳細を記録します。  
成功時は出力引数と直前値をクリアします。  
`detail_out` に `NULL` を指定した場合、本引数へはエラー詳細を設定せず、返却しませんが、直前値は更新されます。  
既存コードで詳細が不要な場合は、従来どおり `NULL` を指定できます。

直前値は `cplat_error_get_last()`、要因は `cplat_error_get_cause()` または `cplat_error_is()`、人間可読の文字列は `cplat_error_message()` で取得します。

## シグネチャを変更した API

次の表では、変更点に関係する引数だけを示します。

| API | 旧シグネチャの詳細引数 | 新シグネチャの詳細引数 |
|---|---|---|
| `cplat_path_get_full` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_paths_equal` | `int *equal_out, int *errno_out` | `int *equal_out, cplat_error *detail_out` |
| `cplat_get_temp_dir` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_path_concat_n` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_path_dirname` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_path_strip_extension` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_path_join_n` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_fopen` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_freopen` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_fopen_fmt` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_vfopen_fmt` | `int *errno_out` | `cplat_error *detail_out` |
| `cplat_fopen_temp` | `int *errno_out` | `cplat_error *detail_out` |

今回の追加調査では、次の API に `cplat_error *detail_out` を追加しました。

- `cplat_remove`、`cplat_rename`、`cplat_remove_fmt`、`cplat_vremove_fmt`
- `cplat_open`、`cplat_open_fmt`、`cplat_vopen_fmt`
- `cplat_lseek`、`cplat_close`、`cplat_dup`、`cplat_dup2`、`cplat_read`、`cplat_write`、`cplat_access`、`cplat_access_fmt`、`cplat_vaccess_fmt`
- `cplat_stat`、`cplat_mkdir`、`cplat_makedirs`、`cplat_rmdir`、`cplat_stat_fmt`、`cplat_vstat_fmt`、`cplat_mkdir_fmt`、`cplat_vmkdir_fmt`
- `cplat_getenv`、`cplat_setenv`、`cplat_unsetenv`
- `cplat_file_open`、`cplat_file_write`、`cplat_file_read`、`cplat_file_get_size`、`cplat_file_set_size`、`cplat_file_get_id`、`cplat_file_get_path_id`、`cplat_file_close`
- `cplat_mmap_attach`、`cplat_mmap_get_rwlock`、`cplat_mmap_flush`、`cplat_mmap_detach`

`cplat_file_close` と `cplat_mmap_detach` は、解放処理の失敗を通知するため、戻り値を `void` から共通結果コードへ変更しました。  
`cplat_mmap_get_rwlock` は、ロック ポインターを戻り値から `lock_out` へ移し、戻り値で共通結果コードを返します。  
`cplat_stat` は変換・整形系の引数順序規約に従い、`cplat_stat(buf, detail_out, path)` の順序で呼び出します。

新たに `cplat_fclose`、`cplat_fflush`、`cplat_fread`、`cplat_fwrite`、`cplat_file_flush` を追加しました。  
標準 I/O ラッパーは元の CRT と同じ戻り値規約を保ち、OS エラー詳細だけを抽象化します。

`cplat_getenv`、`cplat_setenv`、`cplat_unsetenv` と文字列コピー API は、生の errno 値ではなく共通結果コードを返します。  
文字列コピー API は OS エラーを発生させないため、`detail_out` を追加していません。

当初の調査では 9 API を対象としていましたが、同じヘッダーにある `cplat_freopen`、`cplat_fopen_temp`、`cplat_paths_equal` を含めると 12 API でした。  
同一ヘッダーに異なる詳細エラー規約を残さないため、12 API を同時に変更しました。

`cplat_path_concat()` と `cplat_path_join()` は、それぞれ `cplat_path_concat_n()` と `cplat_path_join_n()` を呼ぶマクロです。  
これらのマクロへ渡す詳細引数も `cplat_error *` に変わります。

## コンパイルでは検出できない挙動変更

旧 API は成功時に `errno_out` の値を変更しない場合がありました。  
新 API は成功時に `detail_out` とスレッド ローカルの直前値を必ずクリアします。

成功後も以前の失敗値が残ることを前提にしたコードは、コンパイルが成功しても挙動が変わります。  
失敗の詳細を後続 API の呼び出し後も保持する場合は、対象 API の `detail_out` を使用するか、失敗直後に `cplat_error_get_last()` で値をコピーしてください。

## 書き換え例

### 詳細が不要な場合

`NULL` の指定方法は変わりません。

```c
int rc = cplat_path_get_full(path_out, sizeof(path_out), NULL, path);
```

### 生の errno を受け取っていた場合

```c
/* 旧 */
int errno_value = 0;
FILE *file = cplat_fopen(path, "rb", &errno_value);

/* 新 */
cplat_error detail;
FILE *file = cplat_fopen(path, "rb", &detail);
```

### OS に依存しない原因判定

```c
if (file == NULL && cplat_error_is(&detail, CPLAT_CAUSE_NOT_FOUND) != 0)
{
    /* Linux の ENOENT と Windows の ERROR_FILE_NOT_FOUND を同じ意味で扱う。 */
}
```

生の `ENOENT` との比較は、`CPLAT_CAUSE_NOT_FOUND` の判定へ置き換えます。  
同様に、OS ごとのエラー定数を並べた分岐は、対応する `CPLAT_CAUSE_*` がある場合に要因判定へ置き換えます。

### エラー メッセージの取得

```c
char message[256];
cplat_error detail;
FILE *file = cplat_fopen(path, "rb", &detail);
if (file == NULL)
{
    (void)cplat_error_message(message, sizeof(message), &detail);
}
```

旧 `cplat_errno_message()` または `cplat_win32_error_message()` の呼び出しは、詳細値の取り込みと `cplat_error_message()` の組み合わせへ変更します。

### 自前の OS 呼び出し結果を取り込む場合

```c
cplat_error detail;

if (os_operation_failed)
{
    cplat_error_capture_current_errno(&detail);
}
```

明示的に保存した値を取り込む場合は、`cplat_error_capture_errno()` を使用します。  
Windows では、失敗した OS API の直後に `cplat_error_capture_current_windows_error()` を呼び出します。  
明示的に保存した `GetLastError()` の値を取り込む場合は、`cplat_error_capture_windows_error()` を使用します。  
メッセージ生成などの別 API を先に呼ぶと、OS の直前値が上書きされる可能性があります。

生の値が必要な場合は、ドメインを確認した後で `cplat_error_get_errno()` または `cplat_error_get_windows_error()` を使用します。

## 公開エラー文字列 API の変更

`cplat_errno_message()` と `cplat_win32_error_message()` は公開 API から削除しました。  
新しい公開 API は `cplat_error_message()` だけであり、`cplat_error` のドメインに基づいて処理を振り分けます。

生の数値だけを受け取る API は、呼び出し側が errno と Win32 エラー コードを取り違えても型や値から検出できません。  
ドメイン付きの値を境界にすることで、誤ったエラー体系による文字列化を防ぎます。

## TLS の利用と制限

直前値はスレッドごとに独立しており、別スレッドの失敗や成功では変更されません。  
ただし、同じスレッドで次の対応 API を呼び出すと、成功か失敗かにかかわらず更新されます。

確実に保持する必要がある詳細には `detail_out` を使用してください。  
`cplat_error_get_last()` は、既存の API 呼び出し形を保ったまま補助的に詳細を取得する用途に適しています。  
失敗後の後処理によって直前値が変化するアダプターは、失敗の詳細を先にコピーし、後処理後に `cplat_error_set_last()` で復元できます。  
NULL を指定すると、現在のスレッドの直前値をクリアします。

同一プロセスで cplat の静的ライブラリ版と動的ライブラリ版を混在させると、それぞれが別の TLS を持ちます。  
同じ実行ファイル内で両方を使用してはなりません。

## 修正された Windows 固有の問題

旧 `cplat_fopen_temp()` は一部の Windows エラーで `GetLastError()` の値を `int *errno_out` へ格納していました。  
呼び出し側がその値を errno として文字列化すると、無関係なメッセージになる可能性がありました。

新 API は Windows の失敗を `CPLAT_ERROR_DOMAIN_WINDOWS` として保持します。  
`cplat_error_message()` はドメインを確認して Win32 エラーとして文字列化します。

## 移行手順

1. 利用コードをビルドし、12 API の引数型変更によるコンパイル エラーを確認します。
2. `int errno_value` などの受け取り先を `cplat_error detail` へ変更します。
3. `ENOENT` などとの比較を、対応する `CPLAT_CAUSE_*` の判定へ変更します。
4. 生のエラー文字列 API を `cplat_error_message()` へ変更します。
5. 成功後に以前のエラー値が残ることを前提にしていないか確認します。
6. mock の `EXPECT_CALL` と `ON_CALL` の引数型を更新します。
7. 利用モジュールの局所ビルドと局所テストを実行します。

残存参照は、次の検索で確認できます。

```bash
rg -n 'errno_out|cplat_errno_message|cplat_win32_error_message' <対象ディレクトリ>
rg -n 'cplat_(path_get_full|paths_equal|get_temp_dir|path_concat_n|path_dirname|path_strip_extension|path_join_n|fopen|freopen|fopen_fmt|vfopen_fmt|fopen_temp)' <対象ディレクトリ>
```

## 今回の対象外

`sync` と `runtime` の全 API へ `detail_out` を追加する変更は、今回の対象外です。  
これらへ詳細エラー抽象化を展開する場合も、戻り値、`detail_out`、TLS の役割分担と成功時クリアの規約を維持します。
