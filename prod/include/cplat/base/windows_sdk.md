# windows_sdk.h

## 目的

`windows_sdk.h` は、Windows SDK を参照する際の共通ヘッダーです。

主な目的は次のとおりです。

- `winsock2.h` と `windows.h` の取り込み順序を統一します。
- `windows.h` 直読みのばらつきをなくす
- `winsock.h` との衝突を防ぐ
- Windows 固有の前処理マクロ設定を一箇所で管理します。

## 提供する機能

`windows_sdk.h` は Windows 環境で次を行います。

- `WIN32_LEAN_AND_MEAN` を定義します。
- `winsock2.h` を取り込む
- `ws2tcpip.h` を取り込む
- `windows.h` を取り込む
- 必要に応じて `byte` マクロを `#undef` します。

この順序により、`windows.h` が内部で古い `winsock.h` を先に導入してしまう事故を防ぎます。

## 利用ルール

Windows SDK の型や API が必要な場合は、`<windows.h>` を直接取り込まず、`<cplat/base/windows_sdk.h>` を参照します。

想定する利用例は次のようなヘッダーやソースです。

- 同期プリミティブやスレッド抽象の基盤になる Windows 型を使うヘッダー
- `MAX_PATH` を参照するヘッダー
- `HMODULE` や `cplat/sync/sync.h` の基盤型を参照するヘッダー
- `DllMain` や `OutputDebugStringW` を使うヘッダー

## このヘッダーが必要な理由

Windows では、あるヘッダーが先に `windows.h` を取り込み、その後で別のヘッダーが `winsock2.h` を取り込むと、次のような衝突が発生することがあります。

- マクロ再定義
- `sockaddr` などの構造体再定義
- `accept` や `bind` などの関数宣言衝突

この問題は、利用側のソースだけでなく、公開ヘッダー同士の依存順序でも発生します。  
そのため、共通ルールを持つ必要があります。

## 設計方針

- Windows SDK の取り込み順序は `windows_sdk.h` を正本とします。
