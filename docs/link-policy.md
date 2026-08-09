# リンク方式の規約

## 概要

`com_util` は動的ライブラリだけを生成します。

| プラットフォーム | 成果物 |
|---|---|
| Linux | `libcom_util.so` |
| Windows | `libcom_util.dll` / `libcom_util.lib` (import library) |

静的ライブラリ `libcom_util_static.a` / `libcom_util_static.lib` は生成しません。

`com_util` はプロセス グローバルな状態を持つため、すべての利用者を同じ動的ライブラリへ接続します。

## 利用側の記述

利用側は `makepart.mk` で次のように指定します。

```make
LIBS += com_util
```

Windows の利用側で `COM_UTIL_STATIC` を定義してはいけません。

`COM_UTIL_STATIC` は、製品ソースやモックの関数をテスト実行ファイルへ直接定義する場合に `dllimport` を抑止するためだけ使用します。

`libcom_util` は `libcjson.so` または `libcjson.dll` を動的に利用します。

実行時は `libcom_util` と `libcjson` の両方を解決できるようにします。

## 実行ファイルへの同梱

単体配布する CLI や OS のサービスとして起動する実行ファイルには、`libcom_util` と `libcjson` を同じディレクトリへ同梱します。

このリポジトリでは、対象 app の製品ビルドが次の成果物を `prod/cbin` へコピーします。

| プラットフォーム | 同梱するファイル |
|---|---|
| Linux | `libcom_util.so` / `libcjson.so` |
| Windows | `libcom_util.dll` / `libcjson.dll` |

Linux の対象実行ファイルと `libcom_util.so` は、`$ORIGIN` の RUNPATH で同じディレクトリを探索します。

Windows は実行ファイルと同じディレクトリの DLL を標準の探索順序で解決します。

この配置により、systemd や Windows SCM から起動する `service-sample` も、開発者の `LD_LIBRARY_PATH` や `PATH` に依存しません。

`libcjson` を配布物に含める場合は、`app/cjson/README.md` に記載された MIT License の条件に従います。

## 確認方法

Linux では `readelf` と `ldd` を使い、動的リンクと実行時の解決先を確認できます。

```bash
readelf -d <利用側>/prod/cbin/<実行ファイル> | grep -E 'NEEDED|RUNPATH'
env -u LD_LIBRARY_PATH ldd <利用側>/prod/cbin/<実行ファイル> | grep -E 'com_util|cjson'
```

`libcom_util.so` と `libcjson.so` の解決先が、実行ファイルと同じ `prod/cbin` になることを確認します。

Windows では `dumpbin /dependents` を使い、実行ファイルが `libcom_util.dll` を参照することを確認します。

## 関連ドキュメント

- [コーディング規範 (特化事項)](coding-guideline.md)
- [プラットフォーム抽象化ガイドライン](platform-abstraction-guideline.md)
- [makepart.mk の記述](../../../framework/makefw/docs/makeparts.md)
