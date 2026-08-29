# AGENTS.md

## 対象範囲

この文書は、`app/c-platform/` 配下の作業に適用します。  
作業前に、[README.md](README.md)と対象ディレクトリから参照できる詳細文書を確認してください。

## 作業時の入口

- `prod/include/` は、利用者向けの公開 API ヘッダーです。
- `prod/include_internal/` は、ライブラリ内部の共有ヘッダーです。
- `prod/libsrc/` は、C の実装です。
- `test/` は、単体テスト、モック、エクスポート確認です。
- [docs/README.md](docs/README.md)は、発行文書の入口です。
- [docs/api-cheatsheet.md](docs/api-cheatsheet.md)は、公開 API の逆引きです。
- [docs/coding-guideline.md](docs/coding-guideline.md)は、cplat 固有の規範です。

## 公開 API と文書の同期

`prod/include/` の関数、型、定数、マクロ、シグネチャ、またはラッパーの利用方針を変更する場合は、同じ変更で `docs/api-cheatsheet.md` の該当箇所を確認し、コードと文書を常に一致させてください。  
不一致は文書の参考情報ではなく、修正が必要な欠陥として扱ってください。

公開関数を追加、削除、または名称変更する場合は、`test/src/libcplatTest/exportTest/exportTest.cc` の `CPLAT_EXPORT_TABLE_COMMON` と `CPLAT_EXPORT_TABLE_PLATFORM` も同じ変更で確認してください。  
公開関数のシグネチャを変更する場合も、型検査を含むエクスポート テストとの一致を確認してください。

## app 固有の規則

- 一般的な C/C++ 規範は、[共通コーディング規範](../general/docs/coding-guideline.md) に従ってください。
- cplat 固有の結果コード、標準時刻型、制約は、[cplat コーディング規範](docs/coding-guideline.md) に集約してください。
- テスト構成は、[testfw のテスト作成手順](../../framework/testfw/docs/how-to-test.md) に従ってください。
- `bench-io` の測定軸を変更する場合は、`prod/src/cmd/bench-io/benchmark-method.md` と `docs/fileio-api-selection-guideline.md` を同じ変更で確認してください。
- `bench-io` の測定結果は管理対象外です。共有する数値と測定環境は、`docs/fileio-api-selection-guideline.md` に記載してください。
- `bench-io` のレコード型を変更する場合は、パディングの有無と `-Wpadded` の警告を確認してください。
- `bench-hashtable` の測定軸を変更する場合は、`prod/src/cmd/bench-hashtable/benchmark-method.md` と `prod/libsrc/cplat/hashtable/hashtable-storage-allocator.md` を同じ変更で確認してください。
- `bench-hashtable` の測定結果は管理対象外です。共有する数値と測定環境は、`prod/libsrc/cplat/hashtable/hashtable-storage-allocator.md` に記載してください。

## 確認コマンド

```bash
make
make test
```
