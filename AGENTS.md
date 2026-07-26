# AGENTS.md

## 重要事項

- 自動ステージング、コミット禁止。指示があるまでステージング、コミットは行わないこと。
- 思考の断片は英語でもよいが、ユーザーに気づきを与えたり報告する際は日本語を用いること。

## リポジトリ概要

C プロジェクト向けの汎用ユーティリティ ライブラリです。トレース、同期、コンソール入出力、文字列処理、時計、ファイル操作、圧縮、暗号、メモリマップド ファイルなど、複数プロジェクトで再利用できる共通処理をまとめています。Linux / Windows 両プラットフォームでの利用を想定しています。

## 作業時の入口

- `makefile` - ルートの入口
- `prod/include/` - 公開 API (ライブラリ利用者向けヘッダー)
- `prod/include_internal/` - ライブラリ内部共有ヘッダー
- `prod/libsrc/` - ソース ファイル (`.c`)
- `test/` - テスト本体、モック、`makepart.mk`
- `docs/` - 発行ドキュメントの目次、個別トピックの解説

## 主要コマンド

```bash
make
make test
```

## 注意点

- 公開 API (`prod/include/` 配下のヘッダー) に関数を追加・削除・シグネチャ変更した場合は、`test/src/libcom_utilTest/exportTest/exportTest.cc` の `COM_UTIL_EXPORT_TABLE_COMMON` / `COM_UTIL_EXPORT_TABLE_PLATFORM` を同じコミットで見直すこと。反映を怠ると `exportTest.symbol_names_match` が失敗する。
