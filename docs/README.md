# com_util

`com_util` は C プロジェクト向けの汎用ユーティリティ群を提供するライブラリです。  
トレース、同期、コンソール入出力、文字列処理、時計、ファイル操作など、  
複数プロジェクトで再利用できる共通処理をまとめています。Linux / Windows  
両プラットフォームでの利用を想定しています。

> 注意: このリポジトリは単独で動作するライブラリですが、通常は他のサブモジュールと  
> 組み合わせて利用することを想定しています。  
> [c-modernization-kit](https://github.com/Hondarer/c-modernization-kit) に統合された利用例があります。  
> `c-modernization-kit` リポジトリ内の `app/com_util` サブモジュールの統合例を参照してください。

## 主な構成

| 機能 | 詳細 |
|---|---|
| トレース | レベル別トレース出力と複数バックエンド |
| 同期 | Linux / Windows 共通の同期プリミティブ |
| コンソール / プロンプト | 端末入出力と対話補助 |
| 文字列 / 時刻 / ランタイム | CRT 補助、時計、ランタイム支援 |
| 圧縮 / 暗号 / ファイル | 基盤機能を支えるユーティリティ |
| テスト支援 | テスト用の補助 API とユーティリティ |
| プラットフォーム | Linux、Windows |

## クイック スタート

`com_util` をビルドしてテストするには、`make` を利用します。詳細は各サブディレクトリの `makefile` を参照してください。

```sh
# ビルド
make -C app/com_util

# テスト実行
make -C app/com_util test
```

### 使用例 (概念)

以下は `com_util` のヘッダーを利用する概念的な例です。実際の API 名は実装を参照してください。

```c
#include "com_util.h"

int main(void) {
	cu_log_init(CU_LOG_INFO, NULL);
	const char *val = cu_config_get("general", "timeout", "1000");
	cu_log_info("timeout=%s", val);
	cu_log_close();
	return 0;
}
```

### テスト

```sh
# app/com_util 配下でのテスト
make -C app/com_util test
```

## API 仕様書

### Doxygen

<!-- docsfw の仕上がりパスに対する相対リンク。この Markdown からの相対パスではないことに注意 -->
- [com_util (public)](../../../doxygen/com_util_public/index.html)
    - [公開 API (com_util)](../../../doxygen/com_util_public/group__COM__UTIL__PUBLIC__API.html)

### 単一ファイル版

- [com_util (public)](doxybook2_public/README.md)
    - [公開 API (com_util)](doxybook2_public/Modules/group__COM__UTIL__PUBLIC__API.html)

## モジュール仕様書

### Doxygen

<!-- docsfw の仕上がりパスに対する相対リンク。この Markdown からの相対パスではないことに注意 -->
- [com_util (internal)](../../../doxygen/com_util_internal/index.html)
    - [ファイルの一覧](../../../doxygen/com_util_internal/files.html)

### 単一ファイル版

- [com_util (internal)](doxybook2_internal/README.md)
    - [ファイルの一覧](doxybook2_internal/Files/README.md)
    - [カテゴリの一覧](doxybook2_internal/Modules/README.md)

## 関連ドキュメント

\toc depth=-1 exclude-basedir=true exclude="doxybook2_public/*" exclude="doxybook2_internal/*"
