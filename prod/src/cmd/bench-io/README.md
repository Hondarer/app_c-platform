# bench-io

固定レコード長バイナリ ファイルに対して、`cplat` の `stdio` ラッパー API とメモリ マップド ファイル API の性能を比較するベンチマークです。

`app/{サブフォルダー}` 配下のコードで、構造体配列や固定レコード長バイナリ ファイルの入出力を実装するときに、どちらの API を選ぶべきかを実測に基づいて判断するために用意しています。

## ドキュメント

- [API 選定基準](https://github.com/Hondarer/app_c-platform/blob/main/docs/fileio-api-selection-guideline.md) - 実測結果から導いた API の選び分け
- [ベンチマークの測定方法](benchmark-method.md) - 測定軸、計測の仕組み、実行手順、CSV の列

## 測定結果

`--csv` の出力先には `prod/src/cmd/bench-io/measurements/` を使います。  
このディレクトリは `.gitignore` で管理対象外にしています。測定値は実行環境ごとに変わり、リポジトリで共有しても意味を持たないためです。

結論の根拠として引用する数値は [API 選定基準](https://github.com/Hondarer/app_c-platform/blob/main/docs/fileio-api-selection-guideline.md) へ転記します。  
数値を更新するときは、測定環境を明記したうえで同書の表を書き換えてください。

## 実行

```bash
make -C app/c-platform
app/c-platform/prod/cbin/bench-io --dir /var/tmp/bench-io --csv app/c-platform/prod/src/cmd/bench-io/measurements/linux.csv
```

短時間で動作を確認する場合は、対象を絞り込みます。

```bash
app/c-platform/prod/cbin/bench-io --dir /var/tmp/bench-io --sizes 4K,64K --min-ms 100 --trials 3
```

オプションの一覧は `bench-io --help` および [ベンチマークの測定方法](benchmark-method.md) を参照してください。
