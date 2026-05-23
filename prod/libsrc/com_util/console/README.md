# console - コンソール UTF-8 ヘルパー

`console` は、Windows と Linux の両方で文字コード安全なコンソールアプリケーションを作るためのユーティリティです。  

## 目的

Windows では UTF-8 マニフェストだけでは、コンソール出力やパイプ経由の出力で文字化けや扱いの差異が残ることがあります。  
このモジュールは `stdout` と `stderr` を透過的に扱い、呼び出し側が通常の `printf` / `fprintf` を使ったまま UTF-8 出力を維持できるようにします。

- Windows のコンソール (TTY) には `WriteConsoleW` 経由で UTF-16 として出力
- パイプやファイルには UTF-8 バイト列をそのまま転送
- Linux では `com_util_console_init` / `com_util_console_dispose` は no-op
- `stdin` には介入せず、既存の入力コードをそのまま利用可能

## 設計の要点

このモジュールは、CRT の `printf` / `fprintf` を置き換えずに活用する方針です。  
Windows では内部パイプとバックグラウンドスレッドで `stdout` / `stderr` をフックし、出力先に応じて書き戻し方式を切り替えます。

- コンソール出力時は UTF-8 を UTF-16 に変換して `WriteConsoleW` で出力
- パイプやファイル出力時は UTF-8 をそのまま `WriteFile` で転送
- `stdin` は差し替えない

`stdin` を差し替えないのは、Windows コンソールの行編集機能や `SetConsoleMode`、`_getch()` など既存の入力手段を壊さないためです。  
Windows 10 1903 以降では UTF-8 マニフェストと併用することにより、CRT の入力関数も UTF-8 を前提に扱えます。

## 代表 API

### com_util_console_init

コンソールヘルパーを初期化します。

- Windows では `stdout` / `stderr` の差し替えと読み取りスレッドの起動を行う
- Linux では何もしない
- 二重呼び出し時は追加の初期化を行わない
- 初期化失敗時は警告を出して復旧不能な状態にはしない

### com_util_console_dispose

コンソールヘルパーを終了し、関連リソースを解放します。

- Windows では差し替えた `stdout` / `stderr` を元に戻す
- Linux では何もしない
- 未初期化時や複数回呼び出しでも安全
- 通常はライブラリアンロード時の自動解放に任せられる

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

- `stdout` と `stderr` を内部パイプに差し替える
- 出力先がコンソールなら `WriteConsoleW` を使う
- 出力先がパイプやファイルなら UTF-8 をそのまま流す
- `stdin` は差し替えない

### Linux / 非 Windows

- `com_util_console_init` / `com_util_console_dispose` / 内部解放処理はいずれも no-op
- 呼び出し側は分岐不要

## 注意点

- このモジュールが扱うのは出力方向 (`stdout` / `stderr`) が中心です
- 入力方向の UTF-8 対応は UTF-8 マニフェストに依存しますので、UTF-8 マニフェストを設定してください
- Windows 10 1903 未満は、CRT 入力がシステムコードページ依存になるためサポート対象外です
