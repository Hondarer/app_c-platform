# 戻り値統一の移行ガイド (他リポジトリ向け)

> [!NOTE]
> 本書は、旧戻り値規約から共通結果コードへの完了済み移行を説明する履歴文書です。
> 現行規則は [cplat コーディング規範](coding-guideline.md)、現行 API は [API チート シート](api-cheatsheet.md) を参照してください。

## 概要

cplat は戻り値規約を、共通結果コード (`CPLAT_OK` + 負値エラー) へ統一しました。  
本書は、cplat を利用する他リポジトリが旧規約から新規約へ追随する際の対応表と移行手順を示します。  
新規約そのものの定義・判定慣用句・適用対象外の一覧は [`coding-guideline.md`](coding-guideline.md) の「エラー処理と戻り値規約」章を参照してください。本書はそこからの差分 (旧→新の対応表と移行手順) に限定します。

## 新規約の要点

- `CPLAT_OK` (0) のみが成功。それ以外はすべてエラー
- エラーはすべて負値。`CPLAT_ERR_UNKNOWN` (-1) は「-2 以下の分類済みコードに該当しないその他のエラー」
- 判定は `rc != CPLAT_OK` のような名前比較を正とする (`rc < 0` も値としては等価だが非推奨)
- 値は ABI として凍結。詳細は `prod/include/cplat/base/result.h` を参照

## 旧規約 → 新規約の対応表

### 旧 enum 型 (4 系統、int + 名前へ置換)

| モジュール | 旧型 | 旧値 | 新値 |
|---|---|---|---|
| `runtime/memory_lock.h` | `cplat_memory_lock_result_t` | `OK=0` / `EINVAL=-1` 等 | `int` + `CPLAT_OK` / `CPLAT_ERR_*` |
| `mmap` | (内部 enum) | `OK` / `INVALID_ARGUMENT` / `SYSTEM_ERROR` | `int` + `CPLAT_OK` / `CPLAT_ERR_INVALID_ARGUMENT` / `CPLAT_ERR_UNKNOWN` |
| `runtime/process.h` | (内部 enum、5 値) | 個別 | `int` + `CPLAT_OK` / `CPLAT_ERR_*` |
| `sync/sync.h` | (内部 enum、8 値) | 個別 | `int` + `CPLAT_OK` / `CPLAT_ERR_*` |

いずれも `enum` 型は完全に廃止したクリーン ブレークです (移行エイリアスなし)。旧 `enum` 型名やメンバー名を参照しているコードはコンパイル エラーとして検出されます。

### define 定数群 (符号・値の変更)

| モジュール | 旧規約 | 新規約 |
|---|---|---|
| `argparser` (`CPLAT_ARGPARSER_OK` 等) | 正値の列挙 (0..5) | `CPLAT_OK` (0)、詳細は `CPLAT_ERR_INVALID_ARGUMENT` (-2)/`CPLAT_ERR_OUT_OF_MEMORY` (-4)/`CPLAT_ERR_DUPLICATE_DEFINITION` (-11)/`CPLAT_ERR_PARSE` (-12)/`CPLAT_ERR_BUFFER_TOO_SMALL` (-8) へ符号反転。`CPLAT_ARGPARSER_ERROR_*` (詳細コード層) は当時据え置いたが、その後 [`api-consistency-migration.md`](api-consistency-migration.md) の「詳細コードの共通結果コードへの統合」で廃止した |
| `etw` / `eventlog` (`ERR_PARAM`/`ERR_ACCESS`/`ERR_SYSTEM`) | 独自の負値 | `CPLAT_ERR_INVALID_ARGUMENT` (-2) / `CPLAT_ERR_PERMISSION_DENIED` (-5) / `CPLAT_ERR_UNKNOWN` (-1) |

### 素の 0/-1 群 (最大勢力、値は互換)

`crt/sys/stat.h`、`crt/file.h`、`crt/time.h`、`crt/path.h` (get_full 系)、`clock.h`、`compress.h`、`crypto.h`、`console.h` (write)、`prompt/pinned_prompt.h` (status 系)、`runtime/module.h`、`runtime/process.h` (get_executable_path)、`runtime/shutdown.h` (register 系)、`runtime/elevated_process.h` (is_elevated/run_if_needed/run_with_result/report_result)、`runtime/sym_loader.h` (info)、`trace/tracer.h`、sink_write 系と `cplat_syslog_sink_rename` が該当します。

これらは **成功 0 / 失敗 -1 のまま値互換** です。`CPLAT_OK == 0`、`CPLAT_ERR_UNKNOWN == -1` のため、`rc != 0` や `rc < 0` による判定は変更なしで動作します。  
分類が精密化された関数 (例: `cplat_path_get_full` と `cplat_process_get_executable_path` の `CPLAT_ERR_BUFFER_TOO_SMALL`、`cplat_pinned_prompt_status_set` の `CPLAT_ERR_OUT_OF_MEMORY`、`cplat_tracer_get_name` の `CPLAT_ERR_INVALID_ARGUMENT` 等) でのみ、`rc == -1` のような数値リテラル比較をしているコードは要見直しです。

### 三値・逆向き API (シグネチャ変更を伴う、最重要)

| API | 旧シグネチャ / 意味 | 新シグネチャ / 意味 |
|---|---|---|
| `cplat_paths_equal` | `(lhs, rhs, int *errno_out)`。戻り値 1=一致/0=不一致/-1=失敗 | `(lhs, rhs, int *equal_out, int *errno_out)`。戻り値は結果コード、真偽は `equal_out` (成功時のみ有効) |
| `cplat_console_attach_parent` | `(argc, argv)`。戻り値 1=再接続/0=何もせず/-1=失敗 | `(argc, argv, int *attached_out)`。戻り値は結果コード、真偽は `attached_out` |
| `cplat_prompt_readline`/`_fmt`/`_at`/`_fmt_at`、`cplat_pinned_prompt_readline`/`_fmt` 系 | **シグネチャは不変**。戻り値 1=入力確定/0=EOF・Ctrl+C・失敗など | 戻り値の意味だけ反転。`CPLAT_OK`=入力確定、`CPLAT_ERR_EOF`=EOF、`CPLAT_ERR_CANCELED`=Ctrl+C、`CPLAT_ERR_INVALID_ARGUMENT`=引数不正 |
| `cplat_shutdown_invoke_for_test` / `cplat_shutdown_request_invoke_for_test` (テスト専用) | `(event)`。戻り値 0=実行/1=実行済み/-1=引数不正 | `(event, int *invoked_out)`。戻り値は結果コード、実行有無は `invoked_out` |
| `cplat_elevated_process_extract_result_target` | `(argc, argv)`。戻り値 1=検出/0=未検出 | `(argc, argv, int *detected_out)`。戻り値は常に `CPLAT_OK`、検出有無は `detected_out` |

**`cplat_prompt_readline` 系は特に注意してください。** シグネチャが変わらないため、旧来の `if (readline(...))` や `== 0`/`!= 0` の真偽値判定は **コンパイルは通ったまま意味が反転** します。呼び出し元をすべて洗い出し、`== CPLAT_OK` / `!= CPLAT_OK` の明示比較へ書き換えてください。

## CPLAT_ERR_NOT_FOUND の新設 (追加の破壊的変更)

`CPLAT_ERR_NOT_FOUND` (-6) を「引数・状態・権限」の帯へ新設しました。  
「対象が存在しない」失敗を、それまでの `CPLAT_ERR_UNKNOWN` から独立した分類として表します。

### 戻り値が変わる条件

OS エラー値を共通結果コードへ写像する `cplat_result_from_errno()` と `cplat_result_from_windows_error()` の対応先を、以下のとおり変更しました。

| プラットフォーム | OS エラー値 | 旧 | 新 |
|---|---|---|---|
| Linux | `ENOENT` | `CPLAT_ERR_UNKNOWN` (-1) | `CPLAT_ERR_NOT_FOUND` (-6) |
| Windows | `ERROR_FILE_NOT_FOUND` | `CPLAT_ERR_UNKNOWN` (-1) | `CPLAT_ERR_NOT_FOUND` (-6) |
| Windows | `ERROR_PATH_NOT_FOUND` | `CPLAT_ERR_UNKNOWN` (-1) | `CPLAT_ERR_NOT_FOUND` (-6) |

上記の写像は `cplat_error_report_errno()` と `cplat_error_report_windows_error()` を経由して戻り値へ反映されます。  
したがって、これらを内部で使用する API が「存在しない対象」に対して返す値が `CPLAT_ERR_UNKNOWN` から `CPLAT_ERR_NOT_FOUND` へ変わります。  
存在しないパスのオープン (`cplat_file_open` の `CPLAT_FILE_OPEN_READ` 単独指定など) や、存在しないパスへの `stat` (`cplat_file_get_path_id` など) が該当します。  
`cplat_error` の `result` メンバーにも同じ値が記録されるため、詳細エラー経由で結果コードを読み出している箇所も同様に変わります。

CRT ラッパー (`cplat_fopen` など、「対象外カテゴリ」に挙げた API) は元 API の戻り値規約を保存するため、戻り値そのものは変わりません。

### 利用者側の対処

シグネチャは変わらないため、**コンパイル エラーとしては検出できません**。  
`CPLAT_ERR_UNKNOWN` との比較で「見つからない」を判定していた箇所を grep で洗い出してください。

```bash
grep -rn 'CPLAT_ERR_UNKNOWN' <対象ディレクトリ>
```

見つからないことだけを区別していた判定は、比較先を差し替えます。

```c
/* 旧 */
if (ret == CPLAT_ERR_UNKNOWN)
{
    /* 対象が存在しない場合の処理 */
}

/* 新 */
if (ret == CPLAT_ERR_NOT_FOUND)
{
    /* 対象が存在しない場合の処理 */
}
```

`CPLAT_ERR_UNKNOWN` を「分類できないその他のエラー」の受け皿として使っていた判定は、書き換えが不要です。  
ただし、その分岐が「見つからない」も併せて拾っていた場合は、`CPLAT_ERR_NOT_FOUND` の分岐が先に必要かどうかを確認してください。

### 要因コードは変更なし

要因コード `CPLAT_CAUSE_NOT_FOUND` の値と対応する OS エラー値は、従来から変わっていません。  
`cplat_error_get_cause()` や `cplat_error_is(&err, CPLAT_CAUSE_NOT_FOUND)` で「見つからない」を判定していた箇所は、修正が不要です。

> [!NOTE]
> 今回の変更は、要因コードでしか区別できなかった「見つからない」を、戻り値だけでも区別できるようにするものです。
> 要因コードは詳細エラーの解釈専用であり、関数の戻り値には使用しません (詳細は [`coding-guideline.md`](coding-guideline.md) の「詳細分類の扱い」を参照してください)。

## 移行手順

1. **コンパイル エラー駆動で検出できる箇所**: 旧 `enum` 型名・メンバー名の参照、シグネチャが変わった API (`cplat_paths_equal` 等) の呼び出しはビルドで機械的に検出できます。まずビルドしてエラー箇所を洗い出してください。
2. **`> 0` / 具体値比較の grep**: 符号反転したカテゴリ (旧 enum 群、argparser) では、以下のパターンで判定式を洗い出します。

   ```bash
   grep -nE '(==|!=|>|<)[[:space:]]*[0-9]+\b' <対象ディレクトリ>/*.c
   ```

   `> 0` で「成功」を判定していた箇所は `== CPLAT_OK` へ、具体値との比較は対応する `CPLAT_*` 名への置換が必要です。
3. **`== -1` 判定の確認 (素の 0/-1 群)**: 値は互換のため大半は無修正で動作しますが、分類が精密化された関数を数値リテラルで比較している箇所がないか確認します。

   ```bash
   grep -nE '(==|!=)[[:space:]]*-1\b' <対象ディレクトリ>/*.c
   ```

4. **シグネチャ変更 API の呼び出し箇所修正**: 上表の「三値・逆向き API」を呼び出している箇所をすべて洗い出し、新シグネチャ・新判定式へ書き換えます。特に `cplat_prompt_readline` 系は真偽値判定 (`if (rc)` 等) を個別に確認してください (シグネチャ不変のためコンパイルでは検出できません)。
5. **ローカル テストで確認**: `make -C app/<repo> test` で回帰がないことを確認します。

## 対象外カテゴリ (変更不要)

以下は元 API の規約を保存する設計のため、今回の統一の対象外です。詳細と理由は [`coding-guideline.md`](coding-guideline.md) の「適用対象外」表を参照してください。

- CRT ラッパー (`cplat_fopen`、`cplat_fclose`、`cplat_fread`、`cplat_open`、`cplat_read` 等)
- `cplat_strcpy` 系、`cplat_getenv` (CRT の `strcpy_s` 系規約)
- `cplat_sscanf`、`cplat_utf8_to_wpath` 等 (変換項目数・変換文字数を返す)
- `win32/win32.h` の UTF-8 ラッパー
- ハンドル生成系 (`*_create`)、値をそのまま返す getter、`void` 関数

## 関連ガイド

OS 由来の詳細値を `int *errno_out` からドメイン付きの `cplat_error` へ移行する手順は、[`error-detail-migration.md`](error-detail-migration.md) を参照してください。
