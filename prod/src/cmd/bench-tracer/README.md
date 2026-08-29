# bench-tracer

tracer の破棄経路と、複数プロセスが同じファイルをローテーションする場合の所要時間を測定します。  
既定では、出力先のない tracer、書き込みごとに永続化を要求するファイル、OS バッファーを使うファイルを比較します。

```sh
app/c-platform/prod/cbin/bench-tracer /var/tmp/bench-tracer.log 16 1000
```

引数は順に、ログ ファイル、プロセス数、プロセスごとの行数です。  
Linux と Windows の同じファイル システム種別、同じプロセス数、同じ行数で比較してください。  
測定区間には子プロセスの起動と終了を含みます。

`file_durable` は既定の耐久性を維持する経路です。  
`file_buffered` は `CPLAT_TRACE_FILE_SINK_OS_BUFFERED` を指定し、書き込みの永続化を OS に委ねる経路です。  
速度差だけで既定動作を変更せず、障害時に直近のログを失ってもよい用途に限って buffered を選択してください。
