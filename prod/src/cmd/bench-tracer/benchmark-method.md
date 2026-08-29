# tracer ベンチマークの測定方法

`bench-tracer` は、出力先のない tracer の破棄経路と、複数プロセスによる共有ログへの出力を測定します。

## 測定ケース

- `filtered` は、出力先を設定していない tracer に書き込み API を呼び出します。  
- `file_durable` は、既定の write-through を有効にした共有ファイルへ出力します。  
- `file_buffered` は、`CPLAT_TRACE_FILE_SINK_OS_BUFFERED` を指定した共有ファイルへ出力します。

ファイルのケースでは、各プロセスが 4 KiB ごとに 3 世代のローテーションを行います。  
親プロセスは全 worker を起動してから待機するため、同じファイルに対する競合を含みます。  
測定区間には worker の生成と終了も含まれます。

## 実行例

Linux では次のように実行します。

```sh
app/c-platform/prod/cbin/bench-tracer /var/tmp/bench-tracer.log 16 1000
```

Windows では次のように実行します。

```powershell
app\c-platform\prod\cbin\bench-tracer.exe C:\Temp\bench-tracer.log 16 1000
```

プロセス数を 1、4、16、64 と変え、各条件を複数回測定してください。  
異なる OS を比較するときは、ストレージ、ファイル システム、電源設定、プロセス数、行数を揃えてください。

## 実装判断の根拠

Linux の `access()` はアクセス権を確認する API であり、ファイルの同一性やサイズを取得できません。  
共有ログでは、別プロセスによるローテーションを検出するためにファイルの同一性が必要なので、`stat` 相当の問い合わせを `access()` へ置き換えていません。  
一方、ローテーション世代の存在確認は事前問い合わせを行わず、`rename()` を実行して `ENOENT` を正常な欠番として扱います。  
この方法により、存在確認と操作の間の競合を避けながら、欠番ごとのシステム コールを 2 回から 1 回へ減らします。

- Linux `access(2)`: https://man7.org/linux/man-pages/man2/access.2.html  
- Linux `rename(2)`: https://man7.org/linux/man-pages/man2/renameat2.2.html  
- Linux `open(2)`: https://man7.org/linux/man-pages/man2/open.2.html  
- Windows `CreateFile`: https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea

既定の write-through は障害直前のログを残すために維持します。  
`file_buffered` の結果は、障害時に直近のログを失う可能性を受け入れられる用途でだけ使用してください。
