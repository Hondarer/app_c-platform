# trace backend: etw

`etw` backend は、Windows 上で `trace` の OS トレース出力を担当する backend です。  
通常の利用者は `com_util/trace/tracer.h` を経由して使い、ETW 固有 API が必要な場合だけ `com_util/trace/etw.h` を直接扱います。

## 目的

Windows でトレースを OS 標準のイベント基盤へ流し、外部の ETW consumer から収集・観測できるようにします。

- Windows 標準のトレース経路に統合できる
- アプリケーション ログを Event Tracing for Windows へ送れる
- `trace` 上位からは syslog との差異を意識せずに使える

## 設計の要点

この backend は TraceLogging ベースで実装されています。  
出力イベントは `Trace` イベントとして記録され、主に `Service` と `Message` の情報を持ちます。

- `trace` 上位では OS トレースの出力先として利用される
- Windows では複数の `com_util_tracer_t` があっても、ETW プロバイダー登録は共有される
- 通常のメッセージ出力は `com_util_tracer_write()` 系から透過的に ETW へ流れる
- `COM_UTIL_TRACE_LEVEL_VERBOSE` と `COM_UTIL_TRACE_LEVEL_DEBUG` はどちらも ETW Level 5 として扱われる

## 代表的な使いどころ

### trace.h から使う場合

通常はこちらです。  
`com_util_tracer_set_os_level()` で OS トレースを有効にし、`com_util_tracer_start()` 後に書き込みます。

### etw.h を直接使う場合

ETW セッションの consumer 側をテストしたい場合や、ETW 固有の provider / session API を明示的に使いたい場合に限ります。

## セッション API

`etw.h` には、provider への書き込みだけでなく ETW セッションの補助 API もあります。  
これは `trace` の通常利用というより、ETW を直接扱うテストや診断向けです。

- `com_util_etw_session_check_access()`: セッション開始権限の確認
- `com_util_etw_session_start()`: リアルタイム ETW セッションの開始
- `com_util_etw_session_stop()`: セッション停止と後始末

## 注意点

- Windows 専用です
- ETW セッション開始には権限が必要です
- consumer API を使う場合、`Administrators` または `Performance Log Users` が必要になることがあります
- ETW のイベント サイズには上限があり、極端に長いメッセージは記録されません
- ETW は通常リング バッファー経由で処理され、syslog や同期ファイル書き込みのようなブロッキングを前提にしません

## 使い分け

- アプリケーションから通常のトレースを書きたい: `trace.h`
- ETW 固有の provider や session を直接扱いたい: `etw.h`

`trace` の入口としては、あくまで `trace.h` が中心です。
