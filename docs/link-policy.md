# リンク方式の規約

## 概要

`cplat` は動的ライブラリだけを生成します。

| プラットフォーム | 成果物 |
|---|---|
| Linux | `libcplat.so` |
| Windows | `libcplat.dll` / `libcplat.lib` (import library) |

静的ライブラリ `libcplat_static.a` / `libcplat_static.lib` は生成しません。

`cplat` はプロセス グローバルな状態を持つため、すべての利用者を同じ動的ライブラリへ接続します。

## 利用側の記述

利用側は `makepart.mk` で次のように指定します。

```make
LIBS += cplat
```

Windows の利用側で `CPLAT_STATIC` を定義してはいけません。

`CPLAT_STATIC` は、製品ソースやモックの関数をテスト実行ファイルへ直接定義する場合に `dllimport` を抑止するためだけ使用します。

`libcplat` は `libcjson.so` または `libcjson.dll` を動的に利用します。

実行時は `libcplat` と `libcjson` の両方を解決できるようにします。

## 実行ファイルへの同梱

単体配布する CLI や OS のサービスとして起動する実行ファイルには、`libcplat` と `libcjson` を同じディレクトリへ同梱します。

このリポジトリでは、対象 app の製品ビルドが次の成果物を `prod/cbin` へコピーします。

| プラットフォーム | 同梱するファイル |
|---|---|
| Linux | `libcplat.so` / `libcjson.so` |
| Windows | `libcplat.dll` / `libcjson.dll` |

Linux の対象実行ファイルと `libcplat.so` は、`$ORIGIN` の RUNPATH で同じディレクトリを探索します。

Windows は実行ファイルと同じディレクトリの DLL を標準の探索順序で解決します。

この配置により、systemd や Windows SCM から起動する `service-sample` も、開発者の `LD_LIBRARY_PATH` や `PATH` に依存しません。

`libcjson` を配布物に含める場合は、`app/cjson/README.md` に記載された MIT License の条件に従います。

## 内部 API はエクスポートしない

`libcplat` がエクスポートするのは、`prod/include/` の公開 API だけです。

Linux では `-fvisibility=hidden` でビルドし、`CPLAT_EXPORT` を付けた関数と変数だけが visibility default になります。`prod/libsrc/cplat/exports.map` の `global: cplat_*` は、コンパイル時に hidden となった記号を再公開しません。Windows も同様に、`CPLAT_EXPORT` を付けた記号だけが `libcplat.lib` に現れます。

したがって `prod/include_internal/` で宣言する内部 API (`cplat_result_from_errno`、`cplat_error_report_errno` など) は、ライブラリの利用者からリンクできません。

テストが内部 API に依存する場合は、定義元の `.c` を `makepart.mk` の `ADD_SRCS` へ追加します。テスト対象のソースが間接的に呼ぶ場合も同様です。

```make
# テスト対象が依存するソース ファイル
ADD_SRCS := \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/error.c \
	$(MYAPP_DIR)/prod/libsrc/cplat/base/result.c
```

`ADD_SRCS` へ追加したソースは、ビルド ディレクトリへシンボリック リンクとして取り込まれるため、テスト ディレクトリの `.gitignore` にも同名のファイルを登録します。

エクスポートの有無は `nm` で確認できます。大文字の `T` が公開 API、小文字の `t` が内部 API です。

```bash
nm prod/lib/libcplat.so | grep ' [Tt] cplat_'
```

## 確認方法

Linux では `readelf` と `ldd` を使い、動的リンクと実行時の解決先を確認できます。

```bash
readelf -d <利用側>/prod/cbin/<実行ファイル> | grep -E 'NEEDED|RUNPATH'
env -u LD_LIBRARY_PATH ldd <利用側>/prod/cbin/<実行ファイル> | grep -E 'cplat|cjson'
```

`libcplat.so` と `libcjson.so` の解決先が、実行ファイルと同じ `prod/cbin` になることを確認します。

Windows では `dumpbin /dependents` を使い、実行ファイルが `libcplat.dll` を参照することを確認します。

## 関連ドキュメント

- [cplat API チート シート](api-cheatsheet.md)
- [コーディング規範 (特化事項)](coding-guideline.md)
- [プラットフォーム抽象化ガイドライン](platform-abstraction-guideline.md)
- [makepart.mk の記述](../../../framework/makefw/docs/makeparts.md)
