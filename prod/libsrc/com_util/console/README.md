# console - Windows コンソール設定ヘルパー

`console` は、Windows と Linux の両方で同じ呼び出しコードを使えるコンソール初期化ユーティリティです。

## 目的

Windows 10 1903 以降では、`activeCodePage=UTF-8` マニフェストによりプロセスの ANSI コード ページ (ACP) を UTF-8 にできます。これにより、`argv`、CRT narrow API、Win32 `-A` API を UTF-8 前提で扱えます。

一方、接続先コンソールにはプロセス ACP とは別に入力コード ページと出力コード ページがあります。このモジュールは、Windows のコンソール設定をアプリケーションの UTF-8 前提に合わせ、ANSI エスケープ シーケンスによる色やカーソル制御を使えるようにします。

- コンソール入力コード ページを UTF-8 に設定
- コンソール出力コード ページを UTF-8 に設定
- stdout / stderr の Virtual Terminal Processing を有効化
- Linux では `com_util_console_init` / `com_util_console_dispose` は no-op

## 設計の要点

このモジュールは、CRT の `printf` / `fprintf` を置き換えません。stdout がコンソール (TTY) である場合にのみ、接続先コンソールの状態を確認し、必要な設定を行います。

- すでに UTF-8 のコード ページは変更しない
- 変更前のコード ページとコンソール モードは保存し、通常終了時に復元する
- パイプやファイルへのリダイレクトでは初期化処理を行わない
- stdin / stdout / stderr のハンドルは変更しない

`activeCodePage=UTF-8` マニフェストはプロセス ACP を UTF-8 にする設定です。コンソールの入力コード ページ / 出力コード ページは別の状態であるため、このモジュールでは `SetConsoleCP(CP_UTF8)` / `SetConsoleOutputCP(CP_UTF8)` を引き続き使用します。

## 代表 API

### com_util_console_init

コンソール ヘルパーを初期化します。

- Windows ではコンソール入出力コード ページと VT 処理を設定する
- Linux では何もしない
- 二重呼び出し時は追加の初期化を行わない
- stdout がコンソールでない場合は何もしない

### com_util_console_dispose

コンソール ヘルパーを終了し、変更した状態を元に戻します。

- Windows では変更前のコンソール入出力コード ページとコンソール モードを復元する
- Linux では何もしない
- 未初期化時や複数回呼び出しでも安全
- 通常はライブラリ アンロード時の自動解放に任せられる

## 使い方

呼び出し側は OS ごとの `#ifdef` を書かずに、同じコードで利用できます。

```c
#include <com_util/console/console.h>
#include <stdio.h>

int main(void)
{
    char buf[256];

    com_util_console_init();

    printf("こんにちは\n");
    fprintf(stderr, "警告\n");

    if (fgets(buf, sizeof(buf), stdin) != NULL) {
        printf("input: %s", buf);
    }

    return 0;
}
```

`com_util_console_dispose()` は明示的に呼び出しても構いませんが、通常は必須ではありません。

## プラットフォームごとの動作

### Windows

- stdout がコンソールである場合にのみ初期化する
- `SetConsoleCP(CP_UTF8)` / `SetConsoleOutputCP(CP_UTF8)` でコンソール入出力コード ページを UTF-8 にする
- stdout / stderr の `ENABLE_VIRTUAL_TERMINAL_PROCESSING` を有効化する
- 変更前の状態を通常終了時に復元する

### Linux / 非 Windows

- `com_util_console_init` / `com_util_console_dispose` / 内部解放処理はいずれも no-op
- 呼び出し側は分岐不要

## 注意点

- Windows では `activeCodePage=UTF-8` マニフェストを併用してください
- このモジュールは stdout / stderr のハンドルを変更しません
- Windows 10 1903 未満はサポート対象外です

## 参考リンク

- Microsoft Learn: Use UTF-8 code pages in Windows apps
  https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
  確認日: 2026-06-01。
- Microsoft Learn: Console Code Pages
  https://learn.microsoft.com/en-us/windows/console/console-code-pages
  確認日: 2026-06-01。
- Microsoft Learn: SetConsoleOutputCP function
  https://learn.microsoft.com/en-us/windows/console/setconsoleoutputcp
  確認日: 2026-06-01。
