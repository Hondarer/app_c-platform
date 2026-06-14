---
short-title: "console"
---

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
- `com_util_console_init` は stdin / stdout / stderr のハンドルを変更しない (昇格時の再接続は `com_util_console_attach_parent` が担当する)

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

### com_util_console_attach_parent

昇格起動された場合に、親プロセスのコンソールへ再接続します。

- Windows では `com_util_process_run_elevated_if_needed` が UAC 昇格で自プロセスを再起動した際に付与する引き継ぎフラグを検出する
- `AttachConsole` で親コンソールへ接続し、stdin / stdout / stderr を親コンソール (CONIN$ / CONOUT$) へつなぎ直す
- 検出したフラグは `argv` から取り除き、`argc` を 1 減らす
- Linux では何もせず 0 を返す
- プログラム開始直後、引数解析および `com_util_console_init` より前に呼び出す

この関数は次の仕組みで昇格プロセスの出力を元のコンソールに表示します。UAC 昇格 (`ShellExecuteExW` の `runas` 動詞) では昇格プロセスを別セキュリティ コンテキストで生成するため、親のハンドルを継承できません。そこで親プロセス ID をコマンド ラインで渡し、昇格プロセス側が親コンソールへ接続し直します。親側は昇格プロセスの一時コンソールを隠して起動するため、別ウインドウは表示されません。

```{.mermaid caption="昇格時のコンソール引き継ぎ"}
sequenceDiagram
    participant P as 親プロセス (未昇格)
    participant C as 昇格プロセス
    P->>C: runas + SW_HIDE + 親PIDフラグ
    C->>C: FreeConsole / AttachConsole(親PID)
    C->>C: CONOUT$ / CONIN$ を std へ再接続
    C-->>P: 同一コンソールへ出力
```

昇格直後は、子プロセスの一時コンソール (conhost) の割り当てが非同期に進みます。子プロセスが自前コンソールへ繋がったままの瞬間に `AttachConsole` を呼ぶと `ERROR_ACCESS_DENIED` で失敗します (`AttachConsole` は呼び出し元がすでにコンソールへ接続済みだと失敗します)。この失敗時は標準ハンドルの付け替えを行わず、かつ直前に `FreeConsole` 済みのため、子プロセスはどのコンソールにも繋がらず出力先を失います。これを避けるため、`FreeConsole` と `AttachConsole` を有界リトライし、割り当てが落ち着くまで数回試行します。通常は 1 回目か 2 回目で接続できます。

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
- `com_util_console_init` は stdout / stderr のハンドルを変更しません (昇格時の再接続は `com_util_console_attach_parent` を使用してください)
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
