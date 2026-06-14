---
short-title: "eventlog"
---

# trace backend: eventlog

`eventlog` backend は、Windows 上で `trace` の OS トレース (運用者向け) を担当する backend です。  
OS トレースは Linux では syslog、Windows ではこの EventLog が担当します。  
通常の利用者は `com_util/trace/tracer.h` を経由して使い、EventLog 固有 API が必要な場合だけ `com_util/trace/eventlog.h` を直接扱います。

## 目的

Windows のアプリケーション イベント ログへ運用ログを流し、Event Viewer や監視基盤から参照できるようにします。

- Windows 標準の運用ログ経路 (イベント ログ) に統合できる
- アプリケーション ログを Event Viewer から確認できる
- `trace` 上位からは syslog との差異を意識せずに使える

## 設計の要点

この backend は `RegisterEventSourceW` / `ReportEventW` / `DeregisterEventSource` を包む層です。  
イベント ソースは com_util 全体で共通とし、ソース名には `COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME` を用います。

- `trace` 上位では OS トレースの出力先として利用される
- Windows では複数の `com_util_tracer` があっても、イベント ソース ハンドルは共有される
- 通常のメッセージ出力は `com_util_tracer_write()` 系から透過的に EventLog へ流れる (既定は無効)
- イベント ソースが共通のため、インスタンス識別名を本文先頭に `[name] ` 形式で付与してインスタンスを判別できるようにする
- 分析性を高めるため、レベル毎に異なるイベント タイプ・イベント ID・カテゴリを割り当てる

### レベルとイベント属性の対応

| トレース レベル | イベント タイプ | イベント ID | カテゴリ |
|---|---|---|---|
| `CRITICAL` | Error | 1 | 1 |
| `ERROR` | Error | 2 | 2 |
| `WARNING` | Warning | 3 | 3 |
| `INFO` | Information | 4 | 4 |
| `VERBOSE` | Information | 5 | 5 |
| `DEBUG` | Information | 6 | 6 |

EventLog のイベント タイプは Error / Warning / Information の 3 種のみですが、レベル毎にイベント ID を分けることで、Event Viewer 側でのフィルターや分析を容易にします。

## イベント ソースの登録

EventLog のイベント ソースは、レジストリ `HKLM\SYSTEM\CurrentControlSet\Services\EventLog\Application\{ソース名}` に登録します。  
この登録は HKLM への書き込みを伴うため、管理者権限 (UAC 昇格) が必要です。  
登録/削除には `eventlog-register` コマンドを使います。

```text
eventlog-register install     共通イベント ソースを登録する (UAC 昇格)
eventlog-register uninstall   共通イベント ソースの登録を削除する (UAC 昇格)
```

`eventlog-register` は未昇格で起動された場合に UAC を要求して自身を再実行し、昇格プロセスは親コンソールを引き継いで結果を表示します。

ソース未登録でも `ReportEventW` 自体は成功しますが、Event Viewer 上ではソース名の解決が行われません。  
また本 backend はメッセージ リソース DLL を提供しないため (EventMessageFile 未設定)、Event Viewer ではイベント ID の説明が見つからない旨とともにメッセージ本文が併記されます。

## 代表的な使いどころ

### trace.h から使う場合

通常はこちらです。  
`com_util_tracer_set_os_level()` で OS トレースを有効にし、`com_util_tracer_start()` 後に書き込みます。  
事前に `eventlog-register install` でイベント ソースを登録しておきます。

### eventlog.h を直接使う場合

イベント ソースの登録/削除 API (`com_util_eventlog_register_source()` / `com_util_eventlog_unregister_source()`) を明示的に使いたい場合に限ります。  
通常はこれらを `eventlog-register` コマンド経由で呼び出します。

## 注意点

- Windows 専用です
- イベント ソースの登録/削除には管理者権限が必要です
- メッセージ リソース DLL は提供しないため、Event Viewer の表示にはイベント ID の説明が見つからない旨が併記されます
- イベント ログのメッセージ サイズには上限があり、極端に長いメッセージは記録されません

## 使い分け

- アプリケーションから通常のトレースを書きたい: `trace.h`
- イベント ソースの登録/削除を行いたい: `eventlog-register` コマンド (内部で `eventlog.h` を使用)

`trace` の入口としては、あくまで `trace.h` が中心です。
