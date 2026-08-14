---
short-title: "eventlog"
---

# trace backend: eventlog

`eventlog` backend は、Windows 上で `trace` の OS トレース (運用者向け) を担当する backend です。  
OS トレースは Linux では syslog、Windows ではこの EventLog が担当します。  
通常の利用者は `com_util/trace/tracer.h` を経由して使い、EventLog 固有 API が必要な場合だけ `com_util/trace/eventlog.h` を直接扱います。

## 目的

Windows のアプリケーション イベント ログへ運用ログを流し、Event Viewer や監視基盤から参照できるようにします。

- Windows 標準の運用ログ経路 (イベント ログ) に統合できます。
- アプリケーション ログを Event Viewer から確認できます。
- `trace` 上位からは syslog との差異を意識せずに使える

## 設計の要点

この backend は `RegisterEventSourceW` / `ReportEventW` / `DeregisterEventSource` を包む層です。  
イベント ソースは com_util 全体で共通とし、ソース名には `COM_UTIL_TRACER_DEFAULT_PROVIDER_NAME` を用います。

- `trace` 上位では OS トレースの出力先として利用される
- Windows では複数の `com_util_tracer` があっても、イベント ソース ハンドルは共有される
- 通常のメッセージ出力は `com_util_tracer_write()` 系から透過的に EventLog へ流れる (既定は無効)
- イベント ソースが共通のため、インスタンス識別名を本文先頭に `[name] ` 形式で付与してインスタンスを判別できるようにします。
- `ReportEventW` には置換文字列を 5 件渡し、メッセージ文字列、実行体ファイル パス、ファイル識別子、インスタンス名、インスタンス識別子を EventData に残す
- 実行ファイル絶対パスは初回書き込み時に 1 度だけ解決し、以後はキャッシュを使用します。
- 分析性を高めるため、レベル毎に異なるイベント タイプ・イベント ID・カテゴリを割り当てる

### レベルとイベント属性の対応

| トレース レベル | イベント タイプ | イベント ID (識別子なし) | カテゴリ |
|---|---|---|---|
| `CRITICAL` | Error | 0x1001 | 1 |
| `ERROR` | Error | 0x1002 | 2 |
| `WARNING` | Warning | 0x1003 | 3 |
| `INFO` | Information | 0x1004 | 4 |
| `VERBOSE` | Information | 0x1005 | 5 |
| `DEBUG` | Information | 0x1006 | 6 |

EventLog のイベント タイプは Error / Warning / Information の 3 種のみですが、レベルと表示形式毎にイベント ID を分けることで、Event Viewer 側でのフィルターや分析を容易にします。

イベント ID は 0x1000 番台に置きます。これは、メッセージ ファイル内でカテゴリ メッセージを 1 から CategoryCount までの ID に固定配置する必要があり、イベント メッセージの ID 空間と分離するためです。ファイル識別子またはインスタンス識別子が 0 以外の場合は、末尾の `_0` を出さないために別のイベント ID を使います。

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
メッセージ テーブル リソース (MESSAGETABLE) は `eventlog-register.exe` に埋め込んであり、登録時に EventMessageFile と CategoryMessageFile へ自身の絶対パスを設定します。  
これにより Event Viewer は本文 (`[インスタンス名] メッセージ`) とカテゴリ名のみを表示し、イベント ID の説明が見つからない旨の補完文は表示しません。  
Event Viewer の「全般」では、実行体ファイル パス、インスタンス名、メッセージ文字列を順に表示し、メッセージ文字列の手前に空行を 1 行入れて区切ります。ファイル識別子またはインスタンス識別子が 0 以外の場合は、それぞれ `_識別子` を付与します。イベント XML には 5 件の EventData が記録されるため、ログ収集側は各値を個別に参照できます。

### カテゴリ名表示の制約

`ReportEventW` で渡すカテゴリは classic Event Log API のカテゴリ番号です。  
`CategoryMessageFile` にカテゴリ メッセージを登録しているため、PowerShell の `Get-EventLog` では `CategoryNumber=4`、`Category=INFO` のようにカテゴリ名を解決できます。

```powershell
Get-EventLog -LogName Application -Source 'com_util.tracer' -Newest 1 |
    Select-Object CategoryNumber, Category, Message |
    Format-List
```

一方、Event Viewer GUI の一覧列「タスクのカテゴリ」と `Get-WinEvent` の `TaskDisplayName` は Windows Event Log API 側の task 表示名として扱われます。  
この backend は manifest provider ではなく classic Event Log source として登録しているため、`Get-WinEvent` では `Task=4`、`TaskDisplayName` は空になり、Event Viewer GUI の一覧では `(4)` のように番号で表示されます。

```powershell
Get-WinEvent -FilterHashtable @{
    LogName = 'Application'
    ProviderName = 'com_util.tracer'
} -MaxEvents 1 |
    Select-Object Task, TaskDisplayName, Id, Message |
    Format-List
```

この差異は Event Viewer GUI の表示経路の制約として扱います。

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
- メッセージ テーブル リソースは `eventlog-register.exe` に埋め込み、登録時に EventMessageFile / CategoryMessageFile へ設定します。登録後の Event Viewer は本文とカテゴリ名のみを表示します
- Event Viewer GUI の一覧列「タスクのカテゴリ」は Windows Event Log API 側の task 表示名を使うため、この backend では `(4)` のようにカテゴリ番号で表示されます。カテゴリ名は `Get-EventLog` の `Category` で確認します
- イベント ログのメッセージ サイズには上限があり、極端に長いメッセージは記録されません

## 使い分け

- アプリケーションから通常のトレースを書きたい: `trace.h`
- イベント ソースの登録/削除を行いたい: `eventlog-register` コマンド (内部で `eventlog.h` を使用)

`trace` の入口としては、あくまで `trace.h` が中心です。
