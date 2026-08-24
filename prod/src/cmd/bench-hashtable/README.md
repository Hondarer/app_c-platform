# bench-hashtable

可変長キーと可変長値を持つハッシュ テーブルに対して、ストレージの確保、更新、圧縮、レコード数の変更の所要時間を測定するベンチマークです。

可変長ストレージの確保は空きブロックの個数に比例します。  
capacity を増やしても 1 件あたりの所要時間がほぼ一定であることを、実測で確かめるために用意しています。

## ドキュメント

- [ベンチマークの測定方法](benchmark-method.md) - 測定軸、計測の仕組み、実行手順、CSV の列
- [hashtable 可変長ストレージの管理方式](../../../libsrc/com_util/hashtable/hashtable-storage-allocator.md) - 空きリストの仕組みと計算量

## 測定結果

`--csv` の出力先には `prod/src/cmd/bench-hashtable/measurements/` を使います。  
このディレクトリは `.gitignore` で管理対象外にしています。測定値は実行環境ごとに変わり、リポジトリで共有しても意味を持たないためです。

結論の根拠として引用する数値は [hashtable 可変長ストレージの管理方式](../../../libsrc/com_util/hashtable/hashtable-storage-allocator.md) へ転記します。  
数値を更新するときは、測定環境を明記したうえで同書の表を書き換えてください。

## 実行

```bash
make -C app/com_util
app/com_util/prod/cbin/bench-hashtable --csv app/com_util/prod/src/cmd/bench-hashtable/measurements/linux.csv
```
