# com_util

`com_util` は C プロジェクト向けの汎用ユーティリティ群を提供するライブラリです。
ログ出力、設定ファイル読み取り、文字列操作、時間計測など、複数プロジェクトで再利用
できる共通処理をまとめています。Linux / Windows 両プラットフォームでの利用を想定しています。

> 注意: このリポジトリは単独で動作するライブラリですが、通常は他のサブモジュールと
> 組み合わせて利用することを想定しています。
> [c-modernization-kit](https://github.com/Hondarer/c-modernization-kit) に統合された利用例があります。
> `c-modernization-kit` リポジトリ内の `app/com_util` サブモジュールの統合例を参照してください。

## 特徴

| 機能 | 詳細 |
|---|---|
| ロギング | レベル別ログ出力、スレッドセーフなログハンドラ登録 |
| 設定読み取り | INI 形式などの軽量設定読み取りユーティリティ |
| 文字列処理 | 安全な文字列操作、トリム、分割など |
| 時間計測 | 高精度タイマーと経過時間計測ヘルパー |
| テスト支援 | テスト用のモックやユーティリティ関数 |
| プラットフォーム | Linux、Windows |

## クイックスタート

`com_util` をビルドしてテストするには、ルートの `make` を利用します。詳細は各サブディレクトリの
`makefile` を参照してください。

```sh
# ライブラリビルド
make -C app/com_util

# テスト実行
make -C app/com_util test
```

### 使用例（概念）

以下は `com_util` のヘッダを利用する概念的な例です。実際の API 名は実装を参照してください。

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

## ドキュメント一覧

| ドキュメント | 内容 |
|---|---|
| アーキテクチャ | コード構成・モジュール一覧（存在する場合は `docs/` を参照） |
| API リファレンス | 主要 API の説明 |
| 使用例 | サンプルコードと設定例 |
