# 戻り値統一の移行ガイド (他リポジトリ向け)

## 概要

com_util は戻り値規約を、共通結果コード (`COM_UTIL_OK` + 負値エラー) へ統一しました。  
本書は、com_util を利用する他リポジトリが旧規約から新規約へ追随する際の対応表と移行手順を示します。  
新規約そのものの定義・判定慣用句・適用対象外の一覧は [`coding-guideline.md`](coding-guideline.md) の「エラー処理と戻り値規約」章を参照してください。本書はそこからの差分 (旧→新の対応表と移行手順) に限定します。

## 新規約の要点

- `COM_UTIL_OK` (0) のみが成功。それ以外はすべてエラー
- エラーはすべて負値。`COM_UTIL_ERR_UNKNOWN` (-1) は「-2 以下の分類済みコードに該当しないその他のエラー」
- 判定は `rc != COM_UTIL_OK` のような名前比較を正とする (`rc < 0` も値としては等価だが非推奨)
- 値は ABI として凍結。詳細は `prod/include/com_util/base/result.h` を参照

## 旧規約 → 新規約の対応表

### 旧 enum 型 (4 系統、int + 名前へ置換)

| モジュール | 旧型 | 旧値 | 新値 |
|---|---|---|---|
| `runtime/memory_lock.h` | `com_util_memory_lock_result_t` | `OK=0` / `EINVAL=-1` 等 | `int` + `COM_UTIL_OK` / `COM_UTIL_ERR_*` |
| `mmap` | (内部 enum) | `OK` / `INVALID_ARGUMENT` / `SYSTEM_ERROR` | `int` + `COM_UTIL_OK` / `COM_UTIL_ERR_INVALID_ARGUMENT` / `COM_UTIL_ERR_UNKNOWN` |
| `runtime/process.h` | (内部 enum、5 値) | 個別 | `int` + `COM_UTIL_OK` / `COM_UTIL_ERR_*` |
| `sync/sync.h` | (内部 enum、8 値) | 個別 | `int` + `COM_UTIL_OK` / `COM_UTIL_ERR_*` |

いずれも `enum` 型は完全に廃止したクリーン ブレークです (移行エイリアスなし)。旧 `enum` 型名やメンバー名を参照しているコードはコンパイル エラーとして検出されます。

### define 定数群 (符号・値の変更)

| モジュール | 旧規約 | 新規約 |
|---|---|---|
| `argparser` (`COM_UTIL_ARGPARSER_OK` 等) | 正値の列挙 (0..5) | `COM_UTIL_OK` (0)、詳細は `COM_UTIL_ERR_INVALID_ARGUMENT` (-2)/`COM_UTIL_ERR_OUT_OF_MEMORY` (-4)/`COM_UTIL_ERR_DUPLICATE_DEFINITION` (-11)/`COM_UTIL_ERR_PARSE` (-12)/`COM_UTIL_ERR_BUFFER_TOO_SMALL` (-8) へ符号反転。`COM_UTIL_ARGPARSER_ERROR_*` (詳細コード層) は当時据え置いたが、その後 [`api-consistency-migration.md`](api-consistency-migration.md) の「詳細コードの共通結果コードへの統合」で廃止した |
| `etw` / `eventlog` (`ERR_PARAM`/`ERR_ACCESS`/`ERR_SYSTEM`) | 独自の負値 | `COM_UTIL_ERR_INVALID_ARGUMENT` (-2) / `COM_UTIL_ERR_PERMISSION_DENIED` (-5) / `COM_UTIL_ERR_UNKNOWN` (-1) |

### 素の 0/-1 群 (最大勢力、値は互換)

`crt/sys/stat.h`、`crt/file.h`、`crt/time.h`、`crt/path.h` (get_full 系)、`clock.h`、`compress.h`、`crypto.h`、`console.h` (write)、`prompt/pinned_prompt.h` (status 系)、`runtime/module.h`、`runtime/process.h` (get_executable_path)、`runtime/shutdown.h` (register 系)、`runtime/elevated_process.h` (is_elevated/run_if_needed/run_with_result/report_result)、`runtime/sym_loader.h` (info)、`trace/tracer.h`、sink_write 系と `com_util_syslog_sink_rename` が該当します。

これらは **成功 0 / 失敗 -1 のまま値互換** です。`COM_UTIL_OK == 0`、`COM_UTIL_ERR_UNKNOWN == -1` のため、`rc != 0` や `rc < 0` による判定は変更なしで動作します。  
分類が精密化された関数 (例: `com_util_path_get_full` と `com_util_process_get_executable_path` の `COM_UTIL_ERR_BUFFER_TOO_SMALL`、`com_util_pinned_prompt_status_set` の `COM_UTIL_ERR_OUT_OF_MEMORY`、`com_util_tracer_get_name` の `COM_UTIL_ERR_INVALID_ARGUMENT` 等) でのみ、`rc == -1` のような数値リテラル比較をしているコードは要見直しです。

### 三値・逆向き API (シグネチャ変更を伴う、最重要)

| API | 旧シグネチャ / 意味 | 新シグネチャ / 意味 |
|---|---|---|
| `com_util_paths_equal` | `(lhs, rhs, int *errno_out)`。戻り値 1=一致/0=不一致/-1=失敗 | `(lhs, rhs, int *equal_out, int *errno_out)`。戻り値は結果コード、真偽は `equal_out` (成功時のみ有効) |
| `com_util_console_attach_parent` | `(argc, argv)`。戻り値 1=再接続/0=何もせず/-1=失敗 | `(argc, argv, int *attached_out)`。戻り値は結果コード、真偽は `attached_out` |
| `com_util_prompt_readline`/`_fmt`/`_at`/`_fmt_at`、`com_util_pinned_prompt_readline`/`_fmt` 系 | **シグネチャは不変**。戻り値 1=入力確定/0=EOF・Ctrl+C・失敗など | 戻り値の意味だけ反転。`COM_UTIL_OK`=入力確定、`COM_UTIL_ERR_EOF`=EOF、`COM_UTIL_ERR_CANCELED`=Ctrl+C、`COM_UTIL_ERR_INVALID_ARGUMENT`=引数不正 |
| `_com_util_shutdown_invoke_for_test` / `_com_util_shutdown_request_invoke_for_test` (テスト専用) | `(event)`。戻り値 0=実行/1=実行済み/-1=引数不正 | `(event, int *invoked_out)`。戻り値は結果コード、実行有無は `invoked_out` |
| `com_util_elevated_process_extract_result_target` | `(argc, argv)`。戻り値 1=検出/0=未検出 | `(argc, argv, int *detected_out)`。戻り値は常に `COM_UTIL_OK`、検出有無は `detected_out` |

**`com_util_prompt_readline` 系は特に注意してください。** シグネチャが変わらないため、旧来の `if (readline(...))` や `== 0`/`!= 0` の真偽値判定は **コンパイルは通ったまま意味が反転** します。呼び出し元をすべて洗い出し、`== COM_UTIL_OK` / `!= COM_UTIL_OK` の明示比較へ書き換えてください。

## 移行手順

1. **コンパイル エラー駆動で検出できる箇所**: 旧 `enum` 型名・メンバー名の参照、シグネチャが変わった API (`com_util_paths_equal` 等) の呼び出しはビルドで機械的に検出できます。まずビルドしてエラー箇所を洗い出してください。
2. **`> 0` / 具体値比較の grep**: 符号反転したカテゴリ (旧 enum 群、argparser) では、以下のパターンで判定式を洗い出します。

   ```bash
   grep -nE '(==|!=|>|<)[[:space:]]*[0-9]+\b' <対象ディレクトリ>/*.c
   ```

   `> 0` で「成功」を判定していた箇所は `== COM_UTIL_OK` へ、具体値との比較は対応する `COM_UTIL_*` 名への置換が必要です。
3. **`== -1` 判定の確認 (素の 0/-1 群)**: 値は互換のため大半は無修正で動作しますが、分類が精密化された関数を数値リテラルで比較している箇所がないか確認します。

   ```bash
   grep -nE '(==|!=)[[:space:]]*-1\b' <対象ディレクトリ>/*.c
   ```

4. **シグネチャ変更 API の呼び出し箇所修正**: 上表の「三値・逆向き API」を呼び出している箇所をすべて洗い出し、新シグネチャ・新判定式へ書き換えます。特に `com_util_prompt_readline` 系は真偽値判定 (`if (rc)` 等) を個別に確認してください (シグネチャ不変のためコンパイルでは検出できません)。
5. **ローカル テストで確認**: `make -C app/<repo> test` で回帰がないことを確認します。

## 対象外カテゴリ (変更不要)

以下は元 API の規約を保存する設計のため、今回の統一の対象外です。詳細と理由は [`coding-guideline.md`](coding-guideline.md) の「適用対象外」表を参照してください。

- CRT ラッパー (`com_util_fopen`、`com_util_fclose`、`com_util_fread`、`com_util_open`、`com_util_read` 等)
- `com_util_strcpy` 系、`com_util_getenv` (CRT の `strcpy_s` 系規約)
- `com_util_sscanf`、`com_util_utf8_to_wpath` 等 (変換項目数・変換文字数を返す)
- `win32/win32.h` の UTF-8 ラッパー
- ハンドル生成系 (`*_create`)、値をそのまま返す getter、`void` 関数
