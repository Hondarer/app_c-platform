# trace backend: file

`file` backend は、`trace` の出力をローカル ファイルへ保存するための backend です。  
Windows / Linux の両方で同じ形式のテキスト ログを出力できます。

## 目的

OS トレースとは別に、アプリケーション自身が追跡しやすいログ ファイルを残したいときに使います。  
障害調査や現地保守、収集基盤に載せる前の一次記録に向いています。

- ETW や syslog に依存せずローカルに記録できる
- 両 OS で同じ書式のログを出力できる
- サイズ上限と世代保持でログ肥大化を抑えられる

## 設計の要点

1 行ごとにローカル時刻の ISO8601 タイムスタンプとレベル文字を付けて追記します。

```text
2026-03-31T12:34:56.789+09:00 I service ready
```

レベル文字は次のとおりです。

- `C`: CRITICAL
- `E`: ERROR
- `W`: WARNING
- `I`: INFO
- `V`: VERBOSE
- `D`: DEBUG

## ローテーション

書き込み後にファイル サイズが上限へ達するとローテーションします。  
世代管理は `path`, `path.1`, `path.2`, ... の形式です。

- 現在のファイル: `path`
- 直前の世代: `path.1`
- さらに古い世代: `path.2`, `path.3`, ...

既定値は次のとおりです。

- 最大サイズ: 10 MB
- 保持世代数: 5

## trace からの使い方

通常は `com_util_tracer_set_file_level()` から有効化します。

```c
#include <com_util/trace/tracer.h>

com_util_tracer *tracer = com_util_tracer_create();

com_util_tracer_set_file_level(tracer, "./logs/myapp.log",
                           COM_UTIL_TRACE_LEVEL_INFO, 0, 0);
com_util_tracer_start(tracer);
com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "service ready");
com_util_tracer_dispose(tracer);
```

`max_bytes == 0` の場合は既定サイズ、`generations <= 0` の場合は既定世代数を使います。

## backend 単体の役割

`com_util/trace/trace_file.h` は、ファイル出力だけを独立して扱いたいときの lower layer です。  
`com_util_trace_file_sink_write()` へ明示タイムスタンプを渡すと、外部で確定した時刻をそのままファイル行頭へ反映できます。  
ただし、このリポジトリの通常利用では `trace.h` から設定する使い方を前提にしています。

## 注意点

- ローカル ファイル システムでの利用を前提としています
- NFS / SMB などネットワーク ファイル システム上への出力は非推奨です
- ロック取得にはタイムアウトがありますが、I/O 自体が OS レベルで長時間ブロックする状況は完全には回避できません
- ローテーションはベスト エフォートで行われ、障害時に呼び出し側を無期限に止めない設計です

## 向いている用途

- サービスの常時運用ログ
- 障害時に現地で直接確認したいログ
- OS トレースとは別にファイルを確実に残したいケース
