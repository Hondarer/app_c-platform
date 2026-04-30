# trace - クロスプラットフォーム トレース基盤

`trace` は、Windows と Linux の両方で同じコードからトレースを出力するためのユーティリティです。  
`com_util/trace/tracer.h` を唯一の入口として、OS トレース、ファイル、`stderr` を共通のトレースレベルで扱えます。

## 目的

アプリケーションから見れば、Windows では ETW、Linux では syslog という違いを意識せずにトレースを書けます。  
さらに、OS トレースとは別にファイル出力と `stderr` 出力を併用でき、それぞれに独立したしきい値を設定できます。

- OS ごとのトレース API 差異を吸収する
- OS トレース / ファイル / `stderr` を同じハンドルで管理する
- 出力先ごとに `CRITICAL` から `DEBUG` までのしきい値を分けられる
- 呼び出し側は `trace.h` だけを見ればよい

## 設計の要点

`trace` は、設定フェーズと出力フェーズを分けたライフサイクルを持ちます。  
`com_util_tracer_create()` 直後は stopped 状態で、`com_util_tracer_set_*()` による設定変更が可能です。  
`com_util_tracer_start()` 後は started 状態となり、`com_util_tracer_write()` 系で出力できます。

```text
create -> set_name / set_* -> start -> write -> stop -> dispose
```

started 中は設定変更できず、設定を変える場合は一度 `com_util_tracer_stop()` で stopped に戻します。  
`com_util_tracer_dispose()` は started / stopped のどちらからでも呼べます。  
`com_util_tracer_get_state()` を使うと、handle が `started` / `stopped` / `disposed` のどれかを確認できます。

## トレースレベル

共通レベルは `COM_UTIL_TRACE_LEVEL_CRITICAL` から `COM_UTIL_TRACE_LEVEL_DEBUG` までの 6 段階で、`COM_UTIL_TRACE_LEVEL_NONE` はその出力先を無効化します。

- `COM_UTIL_TRACE_LEVEL_CRITICAL`: 致命的エラー
- `COM_UTIL_TRACE_LEVEL_ERROR`: エラー
- `COM_UTIL_TRACE_LEVEL_WARNING`: 警告
- `COM_UTIL_TRACE_LEVEL_INFO`: 通常の運用情報
- `COM_UTIL_TRACE_LEVEL_VERBOSE`: 詳細な診断情報
- `COM_UTIL_TRACE_LEVEL_DEBUG`: 最も詳細な診断情報
- `COM_UTIL_TRACE_LEVEL_NONE`: 出力しない

Windows ETW と Linux syslog には `VERBOSE` より細かい標準レベルがないため、`COM_UTIL_TRACE_LEVEL_DEBUG` は
OS トレースでは `COM_UTIL_TRACE_LEVEL_VERBOSE` と同じ詳細度として扱われます。  
一方でファイルと `stderr` では `DEBUG` を独立したレベル文字で区別します。

各出力先は「設定レベル以上に重大なメッセージだけを出す」動作です。

## デフォルト動作

`com_util_tracer_create()` 直後の既定値は次のとおりです。

- OS トレース: `COM_UTIL_TRACE_LEVEL_INFO`
- ファイル: `COM_UTIL_TRACE_LEVEL_ERROR`
- `stderr`: `COM_UTIL_TRACE_LEVEL_NONE`

ただし、ファイル出力はパス未設定のままでは有効になりません。  
ファイルを使う場合は `com_util_tracer_set_file_level()` で出力先パスを設定します。

## 出力先

### OS トレース

OS 標準のトレース基盤へ送ります。

- Windows: ETW
- Linux: syslog

通常は `trace.h` 経由で使い、プラットフォームごとの backend を直接触る必要はありません。

### ファイル

ローカルファイルへ 1 行ずつ追記します。  
サイズ上限に達すると `path`, `path.1`, `path.2` の形式でローテーションします。

### stderr

コンソールやサービス起動時の即時確認向けです。  
ローカル時刻の ISO8601 タイムスタンプ付き 1 行テキストで出力されます。

```text
2026-04-02T12:34:56.789+09:00 I application started
2026-04-02T12:34:56.790+09:00 D polling state dump
```

## 代表 API

### `com_util_tracer_create`

トレースハンドルを作成します。  
初期名は実行ファイル名で、取得できない場合は `"unknown"` になります。

### `com_util_tracer_set_name`

識別名を設定します。  
複数インスタンスを識別したい場合は識別番号付きの名前を使えます。

### `com_util_tracer_set_os_level`

OS トレースのしきい値を設定します。

### `com_util_tracer_set_file_level`

ファイル出力先、ファイル用しきい値、最大サイズ、世代数を設定します。  
ファイル出力を使う場合の入口です。

### `com_util_tracer_set_stderr_level`

`stderr` 出力のしきい値を設定します。

### `com_util_tracer_start` / `com_util_tracer_stop`

出力の開始と停止を行います。  
設定変更は stopped 中、書き込みは started 中に行います。

### `com_util_tracer_get_state`

現在の handle 状態を返します。  
`create` 直後と `stop` 後は `STOPPED`、`start` 後は `STARTED`、`NULL` または利用不可状態は `DISPOSED` です。

### `com_util_tracer_write` / `com_util_tracer_writef`

通常のトレースメッセージを書き込みます。  
公開名の `com_util_tracer_write*()` は source location (`[file:line]`) を自動付与する関数風マクロです。  
`timestamp` に `NULL` を渡すと内部で現在時刻を取得し、明示した `com_util_realtime_timestamp_t` を渡すと file / `stderr` 出力と Linux の `SYSLOG_TEST_FD` デバッグ経路にその時刻を使います。  
明示タイムスタンプが不正な場合も出力自体は継続し、内部で現在時刻へ代替します。この場合の戻り値は `-1` です。

### `com_util_tracer_write_hex` / `com_util_tracer_write_hexf`

バイナリデータを HEX テキストとして書き込みます。  
通信データやバッファ内容を確認したいときに使います。  
タイムスタンプ引数の扱いは `com_util_tracer_write` / `com_util_tracer_writef` と同じです。

source location を付けずに生のメッセージを書き込みたい場合は、低レベル関数 `_com_util_tracer_write*()` を直接呼びます。

## 使い方

呼び出し側は backend の違いを書き分けず、同じコードで利用できます。

```c
#include <com_util/trace/tracer.h>

int main(void)
{
    com_util_tracer_t *tracer = com_util_tracer_create();
    if (tracer == NULL) {
        return 1;
    }

    com_util_tracer_set_name(tracer, "myapp", 0);
    com_util_tracer_start(tracer);

    com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "application started");
    com_util_tracer_writef(tracer, COM_UTIL_TRACE_LEVEL_WARNING, NULL, "retry=%d", 3);

    com_util_tracer_stop(tracer);
    com_util_tracer_dispose(tracer);
    return 0;
}
```

ファイル出力を有効にする場合は、start 前に file sink を設定します。

```c
#include <com_util/trace/tracer.h>

com_util_tracer_t *tracer = com_util_tracer_create();

com_util_tracer_set_name(tracer, "myapp", 0);
com_util_tracer_set_os_level(tracer, COM_UTIL_TRACE_LEVEL_WARNING);
com_util_tracer_set_file_level(tracer, "./logs/myapp.log",
                           COM_UTIL_TRACE_LEVEL_INFO, 0, 0);
com_util_tracer_set_stderr_level(tracer, COM_UTIL_TRACE_LEVEL_CRITICAL);
com_util_tracer_start(tracer);

com_util_tracer_write(tracer, COM_UTIL_TRACE_LEVEL_INFO, NULL, "service ready");

com_util_tracer_dispose(tracer);
```

`max_bytes == 0` は既定値、`generations <= 0` も既定値として扱われます。

## プラットフォームごとの動作

### Windows

- OS トレースは ETW を使う
- `trace` 上位では ETW プロバイダ登録を共有する
- `stderr` とファイルは共通の書式で扱う

### Linux

- OS トレースは syslog を使う
- `stderr` とファイルは共通の書式で扱う
- syslog の facility や `ident` は内部で `trace` が管理する

## 注意点

- 設定関数は stopped 状態でのみ使えます
- メッセージ長には共通上限があり、長文は安全な範囲で切り詰められます
- HEX 出力も上限があり、収まらない場合は末尾を省略します
- ファイル出力はローカルファイルシステム向けです
- backend 固有の制約は各 backend README を参照してください

## backend ドキュメント

- `backends/etw/README.md`: Windows ETW backend
- `backends/file/README.md`: ファイル backend
- `backends/syslog/README.md`: Linux syslog backend
