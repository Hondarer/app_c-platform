# com_util から c-platform への移行

## 概要

`com_util` は `c-platform` へ改名しました。  
この変更は互換レイヤーを提供しない破壊的変更です。  
利用側は、アプリ参照、ヘッダー、リンク名、API、mock、および実行時設定を同時に更新してください。

## 名称の対応

| 対象 | 変更前 | 変更後 |
| --- | --- | --- |
| app ディレクトリ | `app/com_util` | `app/c-platform` |
| GitHub リポジトリ | `app_com_util` | `app_c-platform` |
| API 接頭辞 | `com_util_` | `cplat_` |
| C マクロ接頭辞 | `COM_UTIL_` | `CPLAT_` |
| 公開ヘッダー | `<com_util/...>` | `<cplat/...>` |
| 集約ヘッダー | `<com_util.h>` | `<cplat.h>` |
| 共有ライブラリ | `libcom_util` | `libcplat` |
| mock | `mock_com_util` | `mock_cplat` |
| stub | `stub_com_util` | `stub_cplat` |
| tracer 名 | `com_util.tracer` | `c-platform.tracer` |
| 製品用環境変数接頭辞 | `COM_UTIL_` | `C_PLATFORM_` |

## 利用側の更新手順

1. app 依存名と参照パスを `c-platform` へ変更します。  
2. include パスを `<cplat/...>` または `<cplat.h>` へ変更します。  
3. API とマクロの接頭辞を、それぞれ `cplat_` と `CPLAT_` へ変更します。  
4. リンク対象を `libcplat` へ変更します。  
5. テストの mock と stub を `mock_cplat` と `stub_cplat` へ変更します。  
6. コマンド ライン引数、tracer 名、および環境変数を `c-platform` の実行時名称へ変更します。  
7. ビルド、単体テスト、および Doxygen 生成を実行します。

## 互換性を維持する項目

改名によって、関数の処理内容、数値の結果コード、公開構造体のレイアウト、シリアライズ形式、および形式のバージョン値は変更しません。  
Windows の ETW provider GUID、イベント ID、メッセージ ID、および製品名を含まない Win32 ラッパー名も変更しません。

## GitHub リポジトリの移行

GitHub リポジトリの改名は、ローカル変更の検証後に別作業として実施します。  
改名前のローカル検証では、`.gitmodules` と submodule の remote URL は旧リポジトリを参照します。  
GitHub 上で `app_c-platform` へ改名した後に、これらの URL を新しいリポジトリへ更新してください。
